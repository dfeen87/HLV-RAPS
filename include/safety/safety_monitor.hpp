#pragma once

#include "RAPSDefinitions.hpp"
#include "PropulsionPhysicsEngine.hpp"
#include <optional>
#include <array>

class SafetyMonitor {
private:
    PropulsionPhysicsEngine physics_engine_;
    PDTEngine pdt_engine_;

    std::array<RollbackPlan, RAPSConfig::MAX_ROLLBACK_STORE> rollback_store_;
    size_t rollback_count_{0};

    bool check_safety_bounds(const PhysicsState& state) const;

public:
    void init(const PDTEngine& pdt);

    AileeDataPayload validate_policy(const Policy& policy) const;

    AileeStatus run_grace_period(
        const Policy& policy,
        const AileeDataPayload& initial_payload) const;

    bool monitor_execution_integrity(
        const Policy& executed_policy,
        const PhysicsState& current_state) const;

    void commit_rollback_plan(
        const Policy& policy,
        const Policy& safe_fallback_policy);

    std::optional<RollbackPlan> get_last_safe_rollback() const;
};
