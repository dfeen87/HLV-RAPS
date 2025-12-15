#include <iostream>
#include <thread>
#include <chrono>

#include "sil_config.hpp"
#include "sil_supervisor_tests.hpp"

#include "raps/platform/platform_hal.hpp"
#include "raps/supervisor/redundant_supervisor.hpp"

static void sil_sleep_ms(uint32_t ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

int main() {
    std::cout << "========================================================\n";
    std::cout << " RAPS SIL Hardened Runner\n";
    std::cout << "========================================================\n";

    PlatformHAL::seed_rng_for_stubs(SILConfig::RANDOM_SEED);

    RedundantSupervisor supervisor;
    supervisor.init();

    // --- Scenario 1: Nominal cycles ---
    if (SILConfig::VERBOSE_LOGGING) {
        std::cout << "[SIL] Scenario 1: nominal cycles (" << SILConfig::NOMINAL_CYCLES << ")\n";
    }
    sil_test_nominal_cycles(supervisor, SILConfig::NOMINAL_CYCLES);

    // --- Scenario 2: Failover path ---
    if (SILConfig::VERBOSE_LOGGING) {
        std::cout << "[SIL] Scenario 2: failover at cycle " << SILConfig::FAILOVER_AT_CYCLE << "\n";
    }
    sil_test_failover_path(supervisor, SILConfig::NOMINAL_CYCLES, SILConfig::FAILOVER_AT_CYCLE);

    // --- Scenario 3: Prediction mismatch ---
    if (SILConfig::VERBOSE_LOGGING) {
        std::cout << "[SIL] Scenario 3: prediction mismatch detection\n";
    }
    sil_test_prediction_mismatch(supervisor);

    // Optional pacing (keeps SIL closer to RTOS cadence if desired)
    sil_sleep_ms(SILConfig::CYCLE_INTERVAL_MS);

    std::cout << "[SIL] ✅ ALL SCENARIOS PASSED\n";
    return 0;
}
