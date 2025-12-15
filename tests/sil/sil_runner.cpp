#include <iostream>
#include <thread>

#include "raps/supervisor/redundant_supervisor.hpp"
#include "raps/platform/platform_hal.hpp"
#include "raps/core/raps_definitions.hpp"

#include "sil_config.hpp"
#include "sil_safety_cases.hpp"
#include "sil_supervisor_tests.hpp"

int main() {
    std::cout << "[SIL] Starting RAPS Software-in-the-Loop\n";

    PlatformHAL::seed_rng_for_stubs(SILConfig::RANDOM_SEED);

    RedundantSupervisor supervisor;
    supervisor.init();

    PhysicsState state{};
    state.mass_kg = 250000.0f;

    for (uint32_t cycle = 0; cycle < SILConfig::TOTAL_CYCLES; ++cycle) {
        state.timestamp_ms = PlatformHAL::now_ms();

        // Inject faults if enabled
        if (SILConfig::ENABLE_FAULT_INJECTION &&
            cycle == SILConfig::FAULT_AT_CYCLE) {
            inject_execution_failure();
        }

        supervisor.run_cycle(state);

        if (SILConfig::VERBOSE_LOGGING && cycle % 25 == 0) {
            std::cout << "[SIL] Cycle " << cycle << " OK\n";
        }

        std::this_thread::sleep_for(
            std::chrono::milliseconds(SILConfig::CYCLE_INTERVAL_MS)
        );
    }

    std::cout << "[SIL] Completed successfully\n";
    return 0;
}

