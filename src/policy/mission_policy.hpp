#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <optional>
#include <chrono>
#include <limits>

namespace apms::policy {

// -----------------------------
// Common Types
// -----------------------------
using SteadyClock = std::chrono::steady_clock;
using TimePoint   = SteadyClock::time_point;
using Duration    = SteadyClock::duration;

// -----------------------------
// Policy Severity
// -----------------------------
// Severity expresses what should happen when a policy is violated.
// Enforcement is intentionally external to this layer.
//
enum class Severity : uint8_t {
    Advisory = 0,   // Log / observe only
    SoftLimit,      // Graceful clamp or degradation
    HardLimit,      // Immediate constraint enforcement
    Abort           // Mission termination condition
};

// -----------------------------
// Policy Result
// -----------------------------
struct PolicyResult {
    bool violated = false;
    Severity severity = Severity::Advisory;
    std::string message;

    static PolicyResult ok() {
        return {};
    }

    static PolicyResult violation(Severity sev, std::string msg) {
        PolicyResult r;
        r.violated = true;
        r.severity = sev;
        r.message  = std::move(msg);
        return r;
    }
};

// -----------------------------
// Scalar Bounds
// -----------------------------
// Generic scalar constraint (rate, energy, amplitude, temperature, etc.)
//
struct ScalarLimit {
    double min = -std::numeric_limits<double>::infinity();
    double max =  std::numeric_limits<double>::infinity();
    Severity severity = Severity::HardLimit;
    std::string label;

    PolicyResult evaluate(double value) const {
        if (value < min || value > max) {
            return PolicyResult::violation(
                severity,
                label.empty()
                    ? "Scalar limit violated"
                    : ("Scalar limit violated: " + label)
            );
        }
        return PolicyResult::ok();
    }
};

// -----------------------------
// Rate-of-Change Limit
// -----------------------------
// Bounds how quickly a value may change per second.
//
struct SlewRateLimit {
    double max_delta_per_sec = std::numeric_limits<double>::infinity();
    Severity severity = Severity::SoftLimit;
    std::string label;

    PolicyResult evaluate(double previous,
                          double current,
                          Duration dt) const {
        const double secs =
            std::chrono::duration_cast<std::chrono::duration<double>>(dt).count();
        if (secs <= 0.0) return PolicyResult::ok();

        const double rate = std::abs(current - previous) / secs;
        if (rate > max_delta_per_sec) {
            return PolicyResult::violation(
                severity,
                label.empty()
                    ? "Slew-rate limit violated"
                    : ("Slew-rate limit violated: " + label)
            );
        }
        return PolicyResult::ok();
    }
};

// -----------------------------
// Time Window Constraint
// -----------------------------
// Enforces that a condition must not persist longer than allowed.
//
struct DurationLimit {
    Duration max_duration{};
    Severity severity = Severity::HardLimit;
    std::string label;

    PolicyResult evaluate(Duration observed) const {
        if (observed > max_duration) {
            return PolicyResult::violation(
                severity,
                label.empty()
                    ? "Duration limit exceeded"
                    : ("Duration limit exceeded: " + label)
            );
        }
        return PolicyResult::ok();
    }
};

// -----------------------------
// Mission Phase
// -----------------------------
// Phases allow policies to be enabled/disabled contextually.
//
enum class Phase : uint8_t {
    Initialization,
    Standby,
    Active,
    Degraded,
    Shutdown
};

// -----------------------------
// Mission Policy
// -----------------------------
// Declarative container of constraints.
// Evaluation order is deterministic and explicit.
//
struct MissionPolicy {
    std::string mission_name;
    Phase phase = Phase::Initialization;

    // Scalar safety envelopes
    std::vector<ScalarLimit> scalar_limits;

    // Slew / rate limits
    std::vector<SlewRateLimit> slew_limits;

    // Duration-based constraints
    std::vector<DurationLimit> duration_limits;

    // -------------------------
    // Evaluation Helpers
    // -------------------------
    //
    // These helpers are intentionally narrow.
    // The caller owns:
    //   - state tracking
    //   - enforcement actions
    //   - logging / telemetry
    //

    std::vector<PolicyResult>
    evaluate_scalar(double value) const {
        std::vector<PolicyResult> results;
        for (const auto& lim : scalar_limits) {
            auto r = lim.evaluate(value);
            if (r.violated) results.push_back(std::move(r));
        }
        return results;
    }

    std::vector<PolicyResult>
    evaluate_slew(double previous,
                  double current,
                  Duration dt) const {
        std::vector<PolicyResult> results;
        for (const auto& lim : slew_limits) {
            auto r = lim.evaluate(previous, current, dt);
            if (r.violated) results.push_back(std::move(r));
        }
        return results;
    }

    std::vector<PolicyResult>
    evaluate_duration(Duration observed) const {
        std::vector<PolicyResult> results;
        for (const auto& lim : duration_limits) {
            auto r = lim.evaluate(observed);
            if (r.violated) results.push_back(std::move(r));
        }
        return results;
    }
};

// -----------------------------
// Phase-Aware Policy Set
// -----------------------------
// Enables clean switching without mutating policies.
//
struct MissionPolicySet {
    std::vector<MissionPolicy> policies;

    const MissionPolicy* policy_for_phase(Phase p) const {
        for (const auto& pol : policies) {
            if (pol.phase == p) return &pol;
        }
        return nullptr;
    }
};

} // namespace apms::policy
