#pragma once

#include <cmath>
#include <iostream>
#include <limits>

// =====================================================
// Deterministic Safety Monitor (DSM)
// =====================================================
// Hard-physics, last-line-of-defense safety enforcement
// Independent of main control loop
// =====================================================

namespace DSM_Config {

// Absolute physical limits (EFE derived)
constexpr double MAX_CURVATURE_THRESHOLD_RMAX = 1.0e-12;

// HLV Pillar 2: Oscillatory Modulation Stability
constexpr double MIN_ACCEPTABLE_A_T = 0.80;

// HLV Pillar 5: Tri-Cell Coupling
constexpr double MAX_TCC_COUPLING_J = 1.0e+04;

// Failsafe parameters
constexpr double MIN_RESONANCE_AMPLITUDE_CUTOFF = 0.10;

} // namespace DSM_Config

// =====================================================
// DSM Sensor Inputs (Independent Channels)
// =====================================================

struct DsmSensorInputs {
    double measured_proper_time_dilation;
    double measured_oscillatory_prefactor_A_t;
    double measured_tcc_coupling_J;
    double current_resonance_amplitude;
    bool   main_control_system_healthy;
};

// =====================================================
// Deterministic Safety Monitor
// =====================================================

class DeterministicSafetyMonitor {
public:
    enum SafingAction {
        ACTION_NONE          = 0,
        ACTION_ROLLBACK      = 1,
        ACTION_FULL_SHUTDOWN = 2
    };

    DeterministicSafetyMonitor();

    int evaluateSafety(const DsmSensorInputs& inputs);

private:
    double last_estimated_Rmax_;
    bool safing_sequence_active_;

    bool hasInvalidInputs(const DsmSensorInputs& inputs) const;
    bool checkResonanceStability(double A_t, double J_coupling) const;
    double estimateCurvatureScalar(double dilation) const;
    bool checkCurvatureViolation(double R_estimated) const;
};

// =====================================================
// Implementation
// =====================================================

inline DeterministicSafetyMonitor::DeterministicSafetyMonitor()
    : last_estimated_Rmax_(0.0),
      safing_sequence_active_(false) {}

inline double
DeterministicSafetyMonitor::estimateCurvatureScalar(double dilation) const {
    const double R_FACTOR = 1.0e-10;
    double time_stretch = 1.0 - dilation;

    if (time_stretch < 0.0) {
        return std::numeric_limits<double>::infinity();
    }

    return R_FACTOR * time_stretch * time_stretch;
}

inline bool
DeterministicSafetyMonitor::checkCurvatureViolation(double R_estimated) const {
    return (R_estimated >= DSM_Config::MAX_CURVATURE_THRESHOLD_RMAX);
}

inline bool
DeterministicSafetyMonitor::hasInvalidInputs(
    const DsmSensorInputs& inputs
) const {
    return !std::isfinite(inputs.measured_proper_time_dilation) ||
        !std::isfinite(inputs.measured_oscillatory_prefactor_A_t) ||
        !std::isfinite(inputs.measured_tcc_coupling_J) ||
        !std::isfinite(inputs.current_resonance_amplitude);
}

inline bool
DeterministicSafetyMonitor::checkResonanceStability(
    double A_t,
    double J_coupling
) const {
    if (A_t < DSM_Config::MIN_ACCEPTABLE_A_T) {
        std::cerr << "DSM FAILURE PREDICT: A(t) unstable (" << A_t << ")\n";
        return true;
    }

    if (J_coupling > DSM_Config::MAX_TCC_COUPLING_J) {
        std::cerr << "DSM FAILURE PREDICT: TCC coupling exceeded ("
                  << J_coupling << ")\n";
        return true;
    }

    return false;
}

inline int
DeterministicSafetyMonitor::evaluateSafety(
    const DsmSensorInputs& inputs
) {
    if (hasInvalidInputs(inputs)) {
        safing_sequence_active_ = true;
        std::cerr
            << "DSM ALERT: Non-finite sensor input detected — FULL SHUTDOWN\n";
        return ACTION_FULL_SHUTDOWN;
    }

    double R_estimated =
        estimateCurvatureScalar(inputs.measured_proper_time_dilation);

    if (checkCurvatureViolation(R_estimated)) {
        safing_sequence_active_ = true;
        std::cerr
            << "DSM ALERT: ABSOLUTE CURVATURE VIOLATION — FULL SHUTDOWN\n";
        return ACTION_FULL_SHUTDOWN;
    }

    if (checkResonanceStability(
            inputs.measured_oscillatory_prefactor_A_t,
            inputs.measured_tcc_coupling_J
        )) {
        safing_sequence_active_ = true;
        return ACTION_ROLLBACK;
    }

    if (!inputs.main_control_system_healthy &&
        inputs.current_resonance_amplitude >
            DSM_Config::MIN_RESONANCE_AMPLITUDE_CUTOFF) {
        safing_sequence_active_ = true;
        return ACTION_ROLLBACK;
    }

    if (safing_sequence_active_ &&
        R_estimated <
            DSM_Config::MAX_CURVATURE_THRESHOLD_RMAX * 0.5) {
        safing_sequence_active_ = false;
        std::cout << "DSM STATUS: Safety margins restored\n";
    }

    return ACTION_NONE;
}
