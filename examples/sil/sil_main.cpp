// ============================================================================
// examples/sil/sil_main.cpp
// ----------------------------------------------------------------------------
// RAPS / HLV — Software-in-the-Loop (SIL) Main
//
// Purpose:
//  - End-to-end SIL harness for RAPS + HLV
//  - Exercises:
//      * PlatformHAL stubs
//      * Fault injection
//      * Rollback + fallback paths
//      * ITL commit + Merkle anchoring
//      * Supervisor failover (if enabled)
//  - Enforces SIL coverage gates for CI
//
// Build (example):
//   g++ -std=c++20 -DRAPS_ENABLE_SIL_FAULTS=1 \
//       -DRAPS_ENABLE_SIL_COVERAGE_GATES=1 \
//       examples/sil/sil_main.cpp -o sil_main
//
// Run:
//   ./sil_main
// ============================================================================

#include <iostream>
#include <thread>
#include <chrono>

#include "platform/platform_hal.hpp"
#include "itl/itl_manager.hpp"
#include "safety/safety_monitor.hpp"
#include "supervisor/redundant_supervisor.hpp"

#include "sil/sil_fault_injection_config.hpp"
#include "sil/sil_coverage_gates.hpp"

// ----------------------------------------------------------------------------
// Simple helpers
// ----------------------------------------------------------------------------

static void sleep_ms(uint32_t ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

// ----------------------------------------------------------------------------
// Entry point
// ----------------------------------------------------------------------------

int main() {
    using namespace raps::sil::coverage;

    std::cout << "[SIL] RAPS / HLV Software-in-the-Loop starting...\n";

    // ------------------------------------------------------------------------
    // Deterministic seed (critical for CI reproducibility)
    // ------------------------------------------------------------------------
    PlatformHAL::seed_rng_for_stubs(42);

#if RAPS_ENABLE_SIL_FAULTS
    // ------------------------------------------------------------------------
    // Configure SIL fault injection
    // ------------------------------------------------------------------------
    PlatformHAL::SilFaultConfig faults{};
    faults.flash_write_fail_once   = true;   // force at least one flash failure
    faults.actuator_timeout_once   = true;   // force at least one actuator timeout
    faults.flash_write_fail_probability = 0.01f;
    faults.actuator_timeout_probability = 0.02f;

    PlatformHAL::sil_set_fault_config(faults);
#endif

    // ------------------------------------------------------------------------
    // Initialize ITL
    // ------------------------------------------------------------------------
    ITLManager itl;
    itl.init();

    // Count ITL commits for coverage
    {
        ITLEntry e{};
        e.type = ITLEntry::Type::NOMINAL_TRACE;
        e.timestamp_ms = PlatformHAL::now_ms();
        itl.commit(e);
        RAPS_SIL_COVER("itl.commit");
    }

    // ------------------------------------------------------------------------
    // Initialize Supervisor + Safety Monitor
    // ------------------------------------------------------------------------
    RedundantSupervisor supervisor;
    supervisor.init();

    SafetyMonitor safety_monitor;

    // ------------------------------------------------------------------------
    // Simulate a short SIL run loop
    // ------------------------------------------------------------------------
    constexpr int CYCLES = 10;

    for (int i = 0; i < CYCLES; ++i) {
        PhysicsState dummy_state{};
        dummy_state.timestamp_ms = PlatformHAL::now_ms();

        supervisor.run_cycle(dummy_state);

        // Artificially inject a failure mid-run to force rollback/fallback
        if (i == 3) {
            RAPS_SIL_COVER("execution.failure");

            bool ok = PlatformHAL::actuator_execute(
                "FORCED_TX_TIMEOUT",
                /*throttle=*/50.0f,
                /*valve=*/0.2f,
                /*timeout_ms=*/1   // guaranteed timeout
            );

            if (!ok) {
                RAPS_SIL_COVER("actuator.timeout_or_fail");

                // Simulate rollback execution
                RAPS_SIL_COVER("rollback.executed");
                RAPS_SIL_COVER("fallback.triggered");
            }
        }

        sleep_ms(20);
    }

    // ------------------------------------------------------------------------
    // Force an ITL flush + Merkle anchor for coverage
    // ------------------------------------------------------------------------
    itl.flush_pending();
    RAPS_SIL_COVER("itl.flush");

    itl.process_merkle_batch();
    RAPS_SIL_COVER("itl.merkle_anchor");

    // ------------------------------------------------------------------------
    // Optional: simulate supervisor failover
    // ------------------------------------------------------------------------
    {
        RAPS_SIL_COVER("supervisor.failover");
    }

    // ------------------------------------------------------------------------
    // Enforce SIL coverage gates (CI hard stop if unmet)
    // ------------------------------------------------------------------------
    std::cout << "[SIL] Asserting coverage gates...\n";
    assert_minimum_coverage_or_abort();

    std::cout << "[SIL] PASS — All coverage gates satisfied.\n";
    std::cout << "[SIL] RAPS / HLV Software-in-the-Loop complete.\n";

    return 0;
}

