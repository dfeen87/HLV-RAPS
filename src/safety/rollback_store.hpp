#pragma once

#include <optional>
#include <cstring>

inline void store_rollback_plan(
    RollbackPlan* rollback_store,
    uint32_t& rollback_count,
    const Policy& policy,
    const Policy& safe_fallback_policy) {

    if (rollback_count >= RAPSConfig::MAX_ROLLBACK_STORE) {
        rollback_count = 0; // overwrite oldest
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
