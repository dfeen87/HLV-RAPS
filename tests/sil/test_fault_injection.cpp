#include "platform/platform_hal.hpp"

#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <iostream>

static int g_failures = 0;

static void test_case(const char* name, void (*fn)()) {
    try {
        fn();
        std::cout << "✅ " << name << "\n";
    } catch (const std::exception& e) {
        ++g_failures;
        std::cout << "❌ " << name << " (exception): " << e.what() << "\n";
    } catch (...) {
        ++g_failures;
        std::cout << "❌ " << name << " (unknown exception)\n";
    }
}

static void require(bool cond, const char* msg) {
    if (!cond) {
        ++g_failures;
        std::cout << "   ASSERT FAIL: " << msg << "\n";
    }
}

#ifdef RAPS_ENABLE_SIL_FAULTS

static void reset_faults() {
    PlatformHAL::sil_reset_faults();
    // Keep RNG deterministic for reproducible probabilistic tests
    PlatformHAL::seed_rng_for_stubs(12345);
}

static void test_flash_write_fail_once() {
    reset_faults();

    PlatformHAL::SilFaultConfig cfg{};
    cfg.flash_write_fail_once = true;
    PlatformHAL::sil_set_fault_config(cfg);

    uint32_t addr = 0;
    uint8_t data[4] = {1, 2, 3, 4};

    bool first = PlatformHAL::flash_write(addr, data, sizeof(data));
    bool second = PlatformHAL::flash_write(addr, data, sizeof(data));

    require(first == false, "flash_write should fail once when flash_write_fail_once is set");
    require(second == true,  "flash_write should succeed after one-shot fault is consumed");
}

static void test_actuator_timeout_once() {
    reset_faults();

    PlatformHAL::SilFaultConfig cfg{};
    cfg.actuator_timeout_once = true;
    PlatformHAL::sil_set_fault_config(cfg);

    // First call should timeout
    bool first = PlatformHAL::actuator_execute("tx_1", 10.0f, 0.0f, /*timeout_ms=*/5);
    // Second call should behave normally (likely succeed with stub latency)
    bool second = PlatformHAL::actuator_execute("tx_2", 10.0f, 0.0f, /*timeout_ms=*/50);

    require(first == false, "actuator_execute should timeout once when actuator_timeout_once is set");
    require(second == true, "actuator_execute should succeed after one-shot timeout is consumed");
}

static void test_probabilistic_flash_write_failure() {
    reset_faults();

    PlatformHAL::SilFaultConfig cfg{};
    cfg.flash_write_fail_probability = 1.0f; // always fail
    PlatformHAL::sil_set_fault_config(cfg);

    uint8_t data[8] = {0};
    bool ok = PlatformHAL::flash_write(0, data, sizeof(data));
    require(ok == false, "flash_write should fail when flash_write_fail_probability=1.0");
}

static void test_probabilistic_actuator_timeout() {
    reset_faults();

    PlatformHAL::SilFaultConfig cfg{};
    cfg.actuator_timeout_probability = 1.0f; // always timeout
    PlatformHAL::sil_set_fault_config(cfg);

    bool ok = PlatformHAL::actuator_execute("tx_prob", 10.0f, 0.0f, /*timeout_ms=*/1000);
    require(ok == false, "actuator_execute should timeout when actuator_timeout_probability=1.0");
}

#else

static void test_faults_disabled_builds_cleanly() {
    // If we compile without RAPS_ENABLE_SIL_FAULTS, we should still build/run.
    require(true, "fault injection disabled (skipping fault injection tests)");
}

#endif

int main() {
    std::cout << "========================================================\n";
    std::cout << " RAPS SIL Fault Injection Tests\n";
    std::cout << "========================================================\n";

#ifdef RAPS_ENABLE_SIL_FAULTS
    test_case("flash_write_fail_once",               &test_flash_write_fail_once);
    test_case("actuator_timeout_once",               &test_actuator_timeout_once);
    test_case("probabilistic_flash_write_failure",   &test_probabilistic_flash_write_failure);
    test_case("probabilistic_actuator_timeout",      &test_probabilistic_actuator_timeout);
#else
    test_case("faults_disabled_builds_cleanly",      &test_faults_disabled_builds_cleanly);
#endif

    std::cout << "--------------------------------------------------------\n";
    if (g_failures == 0) {
        std::cout << "ALL TESTS PASSED\n";
        return 0;
    }
    std::cout << "TEST FAILURES: " << g_failures << "\n";
    return 1;
}
