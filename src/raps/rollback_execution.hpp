#pragma once

#include <string>

// Executes a rollback plan via the actuator interface.
// Returns true if execution succeeded.
inline bool execute_rollback_plan(
    const RollbackPlan& rollback,
    std::string& out_tx_id) {

    out_tx_id = PlatformHAL::generate_tx_id();

    return PlatformHAL::actuator_execute(
        out_tx_id.c_str(),
        rollback.thrust_magnitude_kN,
        rollback.gimbal_theta_rad,
        RAPSConfig::WATCHDOG_MS / 4
    );
}
