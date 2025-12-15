#pragma once

#include <cmath>

#include "sil_assert.hpp"

#include "raps/supervisor/redundant_supervisor.hpp"
#include "raps/core/raps_definitions.hpp"
#include "raps/platform/platform_hal.hpp"

// Builds a minimal PhysicsState suitable for your run_cycle() usage
inline PhysicsState sil_make_state(uint32_t t_ms) {
    PhysicsState s{};
    s.timestamp_ms = t_ms;
    s.mass_kg = 250000.0f;
    return s;
}

// Scenario 1: Nominal execution should not crash and should complete cycles
inline void sil_test_nominal_cycles(RedundantSupervisor& supervisor, uint32_t cycles) {
    for (uint32_t i = 0; i < cycles; ++i) {
        PhysicsState s = sil_make_state(PlatformHAL::now_ms());
        supervisor.run_cycle(s);
    }
    // If we got here, the loop was stable enough to run.
    SIL_ASSERT_TRUE(true, "Nominal cycles completed");
}

// Scenario 2: Supervisor failover should run and not crash.
// We can’t directly inspect active channel (private), but we *can* verify that
// the failover pathway executes without halting the process.
inline void sil_test_failover_path(RedundantSupervisor& supervisor,
                                  uint32_t total_cycles,
                                  uint32_t failover_at_cycle) {
    SIL_ASSERT_TRUE(failover_at_cycle < total_cycles, "Failover cycle must be < total cycles");

    for (uint32_t i = 0; i < total_cycles; ++i) {
        PhysicsState s = sil_make_state(PlatformHAL::now_ms());

        if (i == failover_at_cycle) {
            // Public API: forces failover logic path.
            supervisor.notify_failure(RedundantSupervisor::FailureMode::PRIMARY_CHANNEL_LOCKUP);
        }

        supervisor.run_cycle(s);
    }

    SIL_ASSERT_TRUE(true, "Failover path executed and cycles completed");
}

// Scenario 3: Prediction mismatch detection should return true when position diverges.
// This directly exercises check_a_b_prediction_mismatch() without needing both channels to predict.
inline void sil_test_prediction_mismatch(RedundantSupervisor& supervisor) {
    PredictionResult a{};
    PredictionResult b{};

    // Minimal end states with a large delta (beyond ACCEPT_POSITION_DEV_M)
    a.predicted_end_state.position_m = {0.0f, 0.0f, 0.0f};
    b.predicted_end_state.position_m = {10000.0f, 10000.0f, 10000.0f}; // huge mismatch

    bool mismatch = supervisor.check_a_b_prediction_mismatch(a, b);
    SIL_ASSERT_TRUE(mismatch, "Expected mismatch detection to trigger");
}
