#pragma once

#include <string>
#include <cmath>
#include <limits>

#include "raps/core/raps_core_types.hpp"
#include "platform/platform_hal.hpp"

// Executes a rollback plan via the actuator interface.
// Returns true if execution succeeded.
inline bool execute_rollback_plan(
    const RollbackPlan& rollback,
    std::string& out_tx_id) {

    // 1. Validate the plan itself
    if (!rollback.valid) {
        return false;
    }

    // 2. Validate control inputs (Sanity Checks)
    // Thrust cannot be negative
    if (rollback.thrust_magnitude_kN < 0.0f) {
        return false;
    }

    // Gimbal angles must be finite numbers
    if (!std::isfinite(rollback.gimbal_theta_rad)) {
        return false;
    }

    if (!std::isfinite(rollback.gimbal_phi_rad)) {
        return false;
    }

    out_tx_id = PlatformHAL::generate_tx_id();
    if (out_tx_id.empty()) {
        return false;
    }

    return PlatformHAL::actuator_execute(
        out_tx_id.c_str(),
        rollback.thrust_magnitude_kN,
        rollback.gimbal_theta_rad,
        RAPSConfig::WATCHDOG_MS / 4
    );
}
