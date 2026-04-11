#pragma once

#include <cmath>
#include <cstdint>
#include <iostream>

// =====================================================
// HLV-RAPS Stability Indicator Upgrade (v3.3.0)
// =====================================================

namespace StabilityConfig {

enum class ManeuverClass {
    CRUISE,
    MILD_MANEUVER,
    AGGRESSIVE_MANEUVER
};

struct Thresholds {
    double S_u_min;
    double S_u_warn;
    double S_u_rate_limit;
};

constexpr Thresholds CRUISE_THRESHOLDS = {0.82, 0.86, 0.015};
constexpr Thresholds MILD_THRESHOLDS = {0.78, 0.83, 0.025};
constexpr Thresholds AGGRESSIVE_THRESHOLDS = {0.72, 0.78, 0.040};

inline constexpr Thresholds get_thresholds(ManeuverClass mclass) {
    switch (mclass) {
        case ManeuverClass::MILD_MANEUVER:
            return MILD_THRESHOLDS;
        case ManeuverClass::AGGRESSIVE_MANEUVER:
            return AGGRESSIVE_THRESHOLDS;
        case ManeuverClass::CRUISE:
        default:
            return CRUISE_THRESHOLDS;
    }
}

} // namespace StabilityConfig

// =====================================================
// DSM Integration
// =====================================================

enum class DSMFlagType {
    NONE = 0,
    SU_LOW = 1,
    SU_RATE_VIOLATION = 2,
    SU_HYSTERESIS_TRANSITION = 3
};

struct DSMEvent {
    uint32_t timestamp;
    StabilityConfig::ManeuverClass maneuver_class;
    double S_u;
    double dS_u_dt;
    DSMFlagType flag_type;
};

// =====================================================
// Stability Indicator Core
// =====================================================

class StabilityIndicator {
public:
    StabilityIndicator()
        : last_S_u_(1.0),
          last_timestamp_(0),
          is_safe_mode_(true),
          initialized_(false) {}

    // Pure mathematical computation of S_u
    // S_u = 1 / (1 + phi^2 + chi^3)
    static double compute_Su(double phi, double chi) {
        double phi_sq = phi * phi;
        double chi_cu = chi * chi * chi;
        return 1.0 / (1.0 + phi_sq + chi_cu);
    }

    // Logging hook (purely deterministic and side-effect free besides console out for simulation)
    static DSMEvent emit_dsm_event(
        uint32_t timestamp,
        StabilityConfig::ManeuverClass mclass,
        double S_u,
        double dS_u_dt,
        DSMFlagType flag_type) {

        DSMEvent event = {timestamp, mclass, S_u, dS_u_dt, flag_type};
        // In a real system, this might push to a queue. For now, it returns the struct.
        return event;
    }

    // Stateful wrapper
    DSMEvent update_stability_state(
        uint32_t timestamp,
        StabilityConfig::ManeuverClass mclass,
        double phi,
        double chi) {

        double current_S_u = compute_Su(phi, chi);
        double dS_u_dt = 0.0;

        auto thresholds = StabilityConfig::get_thresholds(mclass);
        DSMFlagType flag_out = DSMFlagType::NONE;

        if (initialized_ && timestamp > last_timestamp_) {
            double dt = static_cast<double>(timestamp - last_timestamp_);
            dS_u_dt = (current_S_u - last_S_u_) / dt;

            if (std::abs(dS_u_dt) > thresholds.S_u_rate_limit) {
                flag_out = DSMFlagType::SU_RATE_VIOLATION;
            }
        }

        // Hysteresis State Machine
        // EnterSafe: S_u >= S_u_warn
        // ExitSafe:  S_u <= S_u_min
        bool transitioned = false;
        if (!is_safe_mode_) {
            if (current_S_u >= thresholds.S_u_warn) {
                is_safe_mode_ = true;
                transitioned = true;
            }
        } else {
            if (current_S_u <= thresholds.S_u_min) {
                is_safe_mode_ = false;
                transitioned = true;
            }
        }

        if (transitioned && flag_out == DSMFlagType::NONE) {
            flag_out = DSMFlagType::SU_HYSTERESIS_TRANSITION;
        }

        if (!is_safe_mode_ && flag_out == DSMFlagType::NONE) {
             // If we are currently unsafe but haven't triggered another flag,
             // we emit SU_LOW to indicate pre-safing boundary violation.
             if (current_S_u <= thresholds.S_u_min) {
                 flag_out = DSMFlagType::SU_LOW;
             }
        }

        // State update
        last_S_u_ = current_S_u;
        last_timestamp_ = timestamp;
        initialized_ = true;

        return emit_dsm_event(timestamp, mclass, current_S_u, dS_u_dt, flag_out);
    }

    bool is_safe() const { return is_safe_mode_; }

private:
    double last_S_u_;
    uint32_t last_timestamp_;
    bool is_safe_mode_;
    bool initialized_;
};
