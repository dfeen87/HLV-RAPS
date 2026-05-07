#include <cmath>
#include <iostream>
#include <limits>

#include "raps/safety/deterministic_safety_monitor.hpp"
#include "itl/itl_manager.hpp"
#include "platform/platform_hal.hpp"

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
    expect_true(!cond, msg);
}

static RollbackPlan make_valid_plan() {
    RollbackPlan plan{};
    plan.valid = true;
    plan.thrust_magnitude_kN = 10.0f;
    plan.gimbal_theta_rad = 0.01f;
    plan.gimbal_phi_rad = -0.01f;
    return plan;
}

static void test_nominal_wnn_is_observational() {
    ITLManager itl;
    itl.init();
    DeterministicSafetyMonitor dsm;
    PhysicsState active_state{};
    RollbackPlan store[1] = {make_valid_plan()};

    const bool rollback = dsm.pollWnnAndEnforce(
        WnnTelemetry{DSM_Config::WNN_MAX_CURVATURE_PROXY * 0.5, 1.0, PlatformHAL::now_ms()},
        itl,
        store,
        1U,
        active_state
    );

    expect_false(rollback, "nominal WNN telemetry does not trigger rollback");
}

static void test_boundary_breaches_trigger_rollback() {
    ITLManager itl;
    itl.init();
    DeterministicSafetyMonitor dsm;
    PhysicsState active_state{};
    RollbackPlan store[1] = {make_valid_plan()};

    const bool curvature_boundary = dsm.pollWnnAndEnforce(
        WnnTelemetry{DSM_Config::WNN_MAX_CURVATURE_PROXY, 1.0, PlatformHAL::now_ms()},
        itl,
        store,
        1U,
        active_state
    );
    expect_true(curvature_boundary, "WNN curvature boundary triggers rollback");

    const bool prefactor_boundary = dsm.pollWnnAndEnforce(
        WnnTelemetry{0.0, DSM_Config::WNN_MIN_OSCILLATORY_PREFACTOR, PlatformHAL::now_ms()},
        itl,
        store,
        1U,
        active_state
    );
    expect_true(prefactor_boundary, "WNN oscillatory lower boundary triggers rollback");
}

static void test_invalid_wnn_fails_closed() {
    ITLManager itl;
    itl.init();
    DeterministicSafetyMonitor dsm;
    PhysicsState active_state{};
    RollbackPlan store[1] = {make_valid_plan()};

    const bool negative_curvature = dsm.pollWnnAndEnforce(
        WnnTelemetry{-1.0, 1.0, PlatformHAL::now_ms()},
        itl,
        store,
        1U,
        active_state
    );
    expect_true(negative_curvature, "negative WNN curvature is invalid and fails closed");

    const bool non_finite_prefactor = dsm.pollWnnAndEnforce(
        WnnTelemetry{0.0, std::numeric_limits<double>::quiet_NaN(), PlatformHAL::now_ms()},
        itl,
        store,
        1U,
        active_state
    );
    expect_true(non_finite_prefactor, "non-finite WNN prefactor is invalid and fails closed");
}

static void test_dsm_full_shutdown_on_nonfinite_curvature_estimate() {
    DeterministicSafetyMonitor dsm;
    DsmSensorInputs inputs{};
    inputs.measured_proper_time_dilation = 1.01;
    inputs.measured_oscillatory_prefactor_A_t = 1.0;
    inputs.measured_tcc_coupling_J = 1.0;
    inputs.current_resonance_amplitude = 0.0;
    inputs.main_control_system_healthy = true;

    const int action = dsm.evaluateSafety(inputs);
    expect_true(
        action == DeterministicSafetyMonitor::ACTION_FULL_SHUTDOWN,
        "reversed proper-time dilation estimate triggers full shutdown"
    );
}

int main() {
    std::cout << "========================================================\n";
    std::cout << " SIL TEST: DSM WNN Enforcement\n";
    std::cout << "========================================================\n";

    PlatformHAL::seed_rng_for_stubs(2026U);

    test_nominal_wnn_is_observational();
    test_boundary_breaches_trigger_rollback();
    test_invalid_wnn_fails_closed();
    test_dsm_full_shutdown_on_nonfinite_curvature_estimate();

    std::cout << "--------------------------------------------------------\n";
    if (g_failures == 0) {
        std::cout << "✅ ALL DSM WNN TESTS PASSED\n";
        return 0;
    }
    std::cout << "❌ FAILURES: " << g_failures << "\n";
    return 1;
}
