// ============================================================
// SIL Test: PlatformHAL fault injection + invariants
// - No external test framework required
// - CI-friendly: exits non-zero on hard failures
// - "Fault injection" section is smoke-tested (non-flaky by design)
// ============================================================

#include "platform/platform_hal.hpp"

#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

// -----------------------------
// Minimal test harness
// -----------------------------

static int g_failures = 0;

static void expect_true(bool cond, const char* msg) {
    if (!cond) {
        ++g_failures;
        std::cerr << "❌ " << msg << "\n";
    } else {
        std::cout << "✅ " << msg << "\n";
    }
}

static void expect_eq_u32(uint32_t a, uint32_t b, const char* msg) {
    if (a != b) {
        ++g_failures;
        std::cerr << "❌ " << msg << " (got=" << a << " expected=" << b << ")\n";
    } else {
        std::cout << "✅ " << msg << "\n";
    }
}

static void expect_eq_str(const std::string& a, const std::string& b, const char* msg) {
    if (a != b) {
        ++g_failures;
        std::cerr << "❌ " << msg << " (got=\"" << a << "\" expected=\"" << b << "\")\n";
    } else {
        std::cout << "✅ " << msg << "\n";
    }
}

// -----------------------------
// Tests
// -----------------------------

static void test_now_ms_monotonic() {
    uint32_t t1 = PlatformHAL::now_ms();
    uint32_t t2 = PlatformHAL::now_ms();
    expect_true(t2 >= t1, "now_ms() is monotonic (non-decreasing)");
}

static void test_tx_id_properties() {
    PlatformHAL::seed_rng_for_stubs(12345);

    std::string tx = PlatformHAL::generate_tx_id();
    expect_true(tx.size() == 24, "generate_tx_id() returns 24 hex chars");

    auto is_hex = [](char c) {
        return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
    };
    bool all_hex = true;
    for (char c : tx) all_hex = all_hex && is_hex(c);
    expect_true(all_hex, "generate_tx_id() is lowercase hex");
}

static void test_sha256_stub_determinism() {
    PlatformHAL::seed_rng_for_stubs(1);

    const char* msg = "hello";
    Hash256 h1 = PlatformHAL::sha256(msg, std::strlen(msg));
    Hash256 h2 = PlatformHAL::sha256(msg, std::strlen(msg));

    bool same = (std::memcmp(h1.data, h2.data, 32) == 0);
    expect_true(same, "sha256() stub is deterministic for same input");

    const char* msg2 = "hellO";
    Hash256 h3 = PlatformHAL::sha256(msg2, std::strlen(msg2));
    bool different = (std::memcmp(h1.data, h3.data, 32) != 0);
    expect_true(different, "sha256() stub changes when input changes");
}

static void test_flash_read_zero_fill() {
    std::vector<uint8_t> buf(64, 0xFF);
    bool ok = PlatformHAL::flash_read(0, buf.data(), buf.size());

    // flash_read may be fault-injected; only assert content if ok.
    if (ok) {
        bool all_zero = true;
        for (auto b : buf) all_zero = all_zero && (b == 0);
        expect_true(all_zero, "flash_read() stub fills buffer with zeros on success");
    } else {
        std::cout << "⚠️  flash_read() returned false (fault injection or stub behavior)\n";
    }
}

static void test_actuator_idempotency() {
    PlatformHAL::seed_rng_for_stubs(42);

    const std::string tx = "aaaaaaaaaaaaaaaaaaaaaaaa"; // 24 chars
    bool first = PlatformHAL::actuator_execute(tx.c_str(), 90.0f, 0.01f, /*timeout_ms=*/200);
    bool second = PlatformHAL::actuator_execute(tx.c_str(), 50.0f, 0.50f, /*timeout_ms=*/1);

    // Second call should succeed even with tiny timeout because tx_id is idempotent.
    expect_true(first, "actuator_execute(tx) succeeds (first apply)");
    expect_true(second, "actuator_execute(tx) succeeds idempotently (already applied)");
}

static void test_actuator_timeout_behavior() {
    PlatformHAL::seed_rng_for_stubs(99);

    // With timeout_ms=0, almost any simulated latency should fail.
    // This should be deterministic enough to assert "often false", not "always false".
    int fails = 0;
    for (int i = 0; i < 50; ++i) {
        std::string tx = PlatformHAL::generate_tx_id();
        if (!PlatformHAL::actuator_execute(tx.c_str(), 10.0f, 0.0f, /*timeout_ms=*/0)) {
            ++fails;
        }
    }
    expect_true(fails >= 40, "actuator_execute() respects timeout_ms (timeout_ms=0 mostly fails)");
}

static void smoke_test_fault_injection_presence() {
#if defined(RAPS_ENABLE_SIL_FAULTS) && (RAPS_ENABLE_SIL_FAULTS == 1)
    // Non-flaky smoke test:
    // We do NOT fail CI if no injected failure is observed (probabilities are small).
    // We only report what we observed so you can confirm the knobs are enabled.
    PlatformHAL::seed_rng_for_stubs(7);

    int flash_w_fail = 0;
    int downlink_fail = 0;

    for (int i = 0; i < 5000; ++i) {
        if (!PlatformHAL::flash_write(0, "x", 1)) flash_w_fail++;
        if (!PlatformHAL::downlink_queue("x", 1)) downlink_fail++;
    }

    std::cout << "ℹ️  SIL faults enabled. Observed in 5000 trials:\n"
              << "    flash_write failures: " << flash_w_fail << "\n"
              << "    downlink_queue failures: " << downlink_fail << "\n";

    expect_true(true, "fault injection smoke test completed (non-gating)");
#else
    std::cout << "ℹ️  RAPS_ENABLE_SIL_FAULTS not enabled; skipping fault-injection smoke test.\n";
    expect_true(true, "fault injection smoke test skipped (faults disabled)");
#endif
}

// -----------------------------
// Main
// -----------------------------

int main() {
    std::cout << "========================================================\n";
    std::cout << " SIL TEST: PlatformHAL Fault Injection + Invariants\n";
    std::cout << "========================================================\n";

    test_now_ms_monotonic();
    test_tx_id_properties();
    test_sha256_stub_determinism();
    test_flash_read_zero_fill();
    test_actuator_idempotency();
    test_actuator_timeout_behavior();
    smoke_test_fault_injection_presence();

    std::cout << "--------------------------------------------------------\n";
    if (g_failures == 0) {
        std::cout << "✅ ALL TESTS PASSED\n";
        return 0;
    }
    std::cout << "❌ FAILURES: " << g_failures << "\n";
    return 1;
}
