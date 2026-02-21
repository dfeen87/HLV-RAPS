// ============================================================
// SIL Test: Rollback Execution Logic
// - Verifies rollback plan validation
// - Verifies correct actuator dispatch
// ============================================================

#include <iostream>
#include <cmath>
#include <limits>
#include <string>

#include "raps/core/raps_core_types.hpp"
#include "platform/platform_hal.hpp"

// We include the implementation-under-test directly because it is a header-only library
#include "raps/rollback_execution.hpp"

static int g_failures = 0;

static void expect_true(bool cond, const char* msg) {
    if (!cond) {
        ++g_failures;
        std::cerr << "❌ " << msg << "\n";
    } else {
        std::cout << "✅ " << msg << "\n";
    }
}

static void expect_false(bool cond, const char* msg) {
    if (cond) {
        ++g_failures;
        std::cerr << "❌ " << msg << "\n";
    } else {
        std::cout << "✅ " << msg << "\n";
    }
}

void test_rollback_validation() {
    RollbackPlan plan{};
    std::string tx_id;

    std::cout << "--- Testing Rollback Validation ---\n";

    // 1. Invalid plan
    // By default, the struct is zero-initialized, so valid might be false depending on compiler/initialization,
    // but we explicitly set it to false to be sure.
    plan.valid = false;
    // We expect this to fail (return false) once validation is added.
    // Currently, without validation, it might succeed if PlatformHAL succeeds with 0 values.
    bool res1 = execute_rollback_plan(plan, tx_id);
    // Note: In current implementation, this will likely return TRUE because there is no check.
    // So this expectation will fail until I fix the code.
    // But for TDD, I write the test expecting the CORRECT behavior.
    expect_false(res1, "execute_rollback_plan fails for invalid plan (valid=false)");


    // 2. Negative thrust
    plan.valid = true;
    plan.thrust_magnitude_kN = -1.0f;
    plan.gimbal_theta_rad = 0.0f;
    bool res2 = execute_rollback_plan(plan, tx_id);
    expect_false(res2, "execute_rollback_plan fails for negative thrust");

    // 3. Infinite gimbal
    plan.thrust_magnitude_kN = 100.0f;
    plan.gimbal_theta_rad = std::numeric_limits<float>::infinity();
    bool res3 = execute_rollback_plan(plan, tx_id);
    expect_false(res3, "execute_rollback_plan fails for infinite gimbal");

    // 4. Valid plan
    plan.valid = true;
    plan.thrust_magnitude_kN = 50.0f;
    plan.gimbal_theta_rad = 0.1f;
    bool res4 = execute_rollback_plan(plan, tx_id);
    expect_true(res4, "execute_rollback_plan succeeds for valid inputs");
    expect_true(tx_id.length() > 0, "tx_id is generated");
}

int main() {
    std::cout << "========================================================\n";
    std::cout << " SIL TEST: Rollback Execution Logic\n";
    std::cout << "========================================================\n";

    // Seed RNG for deterministic behavior of PlatformHAL
    PlatformHAL::seed_rng_for_stubs(12345);

    test_rollback_validation();

    std::cout << "--------------------------------------------------------\n";
    if (g_failures == 0) {
        std::cout << "✅ ALL ROLLBACK TESTS PASSED\n";
        return 0;
    }
    std::cout << "❌ FAILURES: " << g_failures << "\n";
    return 1;
}
