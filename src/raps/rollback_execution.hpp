#pragma once

#include <string>
#include <cmath>
#include <limits>
#include <cstdint>

#include "raps/core/raps_core_types.hpp"
#include "platform/platform_hal.hpp"
#include "safety/rollback_store.hpp"

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
    // Thrust must be finite and cannot be negative.
    if (!std::isfinite(rollback.thrust_magnitude_kN) ||
        rollback.thrust_magnitude_kN < 0.0f) {
        return false;
    }

    // Gimbal angles must be finite numbers.
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

// Triggers an immediate rollback due to WNN constraints breach
inline bool trigger_wnn_immediate_rollback(
    const RollbackPlan* rollback_store,
    uint32_t rollback_count,
    PhysicsState& active_state_pointer
) {
    if (rollback_count == 0U || rollback_store == nullptr) {
        return false;
    }

    if (rollback_count > RAPSConfig::MAX_ROLLBACK_STORE) {
        PlatformHAL::metric_emit(
            "safety.wnn.rollback_rejected",
            1.0f,
            "reason",
            "rollback_count_oob"
        );
        return false;
    }

    const RollbackPlan& latest_plan = rollback_store[rollback_count - 1U];

    std::string tx_id;
    if (!execute_rollback_plan(latest_plan, tx_id)) {
        PlatformHAL::metric_emit(
            "safety.wnn.rollback_rejected",
            1.0f,
            "reason",
            "plan_execution_failed"
        );
        return false;
    }

    // Peek the latest snapshot without destructive reading
    PhysicsState last_valid_snapshot;
    if (StateSnapshotBuffer.try_peek_latest(last_valid_snapshot)) {
        // Point the active state pointer to the last valid state
        active_state_pointer = last_valid_snapshot;
    }

    return true;
}
