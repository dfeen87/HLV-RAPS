#pragma once

#include <optional>
#include <cstring>

#include "raps/telemetry/telemetry_ring_buffer.hpp"
#include "itl/itl_state_snapshot.hpp"

// Continuous, statically allocated snapshot buffer
inline raps::telemetry::TelemetryRingBuffer<PhysicsState, 64> StateSnapshotBuffer;

inline void store_state_snapshot_tick(const PhysicsState& state) {
    StateSnapshotBuffer.try_push(state);
}

inline void store_rollback_plan(
    RollbackPlan* rollback_store,
    uint32_t& rollback_count,
    const Policy& policy,
    const Policy& safe_fallback_policy) {

    if (rollback_count >= RAPSConfig::MAX_ROLLBACK_STORE) {
        PlatformHAL::metric_emit("safety.rollback_store_full", 1.0f);
        // Evict oldest entry by shifting to make room for new entry
        for (size_t i = 1; i < RAPSConfig::MAX_ROLLBACK_STORE; ++i) {
            rollback_store[i - 1] = rollback_store[i];
        }
        rollback_count = static_cast<uint32_t>(RAPSConfig::MAX_ROLLBACK_STORE) - 1;
    }

    RollbackPlan plan{};
    std::strncpy(plan.policy_id, policy.id,
                 sizeof(plan.policy_id) - 1);

    plan.thrust_magnitude_kN = safe_fallback_policy.thrust_magnitude_kN;
    plan.gimbal_theta_rad = safe_fallback_policy.gimbal_theta_rad;
    plan.gimbal_phi_rad = safe_fallback_policy.gimbal_phi_rad;
    plan.valid = true;

    float rollback_data[] = {
        plan.thrust_magnitude_kN,
        plan.gimbal_theta_rad,
        plan.gimbal_phi_rad
    };

    plan.rollback_hash =
        PlatformHAL::sha256(rollback_data, sizeof(rollback_data));

    rollback_store[rollback_count++] = plan;
}

inline std::optional<RollbackPlan> get_last_rollback_plan(
    const RollbackPlan* rollback_store,
    uint32_t rollback_count) {

    if (rollback_count > 0) {
        return rollback_store[rollback_count - 1];
    }
    return std::nullopt;
}
