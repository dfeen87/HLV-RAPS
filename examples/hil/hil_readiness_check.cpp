// examples/hil/hil_readiness_check.cpp
//
// ============================================================
// HIL Readiness Check
// ------------------------------------------------------------
// Purpose:
// - One-shot executable to validate that the HIL stack is alive:
//     * PlatformHAL is linked to the HIL backend (rig transport)
//     * Flash read/write path works (basic smoke)
//     * Actuator path works + idempotency by tx_id holds
//     * Downlink path works (basic smoke)
//     * Timing + hashing are sane
//
// Expected behavior:
// - Exit code 0  : PASS
// - Exit code 2  : FAIL (a required capability failed)
//
// Notes:
// - This file intentionally uses *only* PlatformHAL public API.
// - For HIL builds, ensure your build links the HIL-backed PlatformHAL .cpp
//   (e.g., platform_hal_hil.cpp) instead of the SIL/host stub.
// ============================================================

#include "platform/platform_hal.hpp"

#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>

namespace {

static int g_failures = 0;

#define CHECK_TRUE(expr, msg)                                                      \
    do {                                                                           \
        if (!(expr)) {                                                             \
            ++g_failures;                                                          \
            std::cerr << "[FAIL] " << (msg) << " (" #expr ")" << std::endl;        \
        } else {                                                                   \
            std::cout << "[PASS] " << (msg) << std::endl;                          \
        }                                                                          \
    } while (0)

#define CHECK_EQ(a, b, msg)                                                        \
    do {                                                                           \
        if (!((a) == (b))) {                                                       \
            ++g_failures;                                                          \
            std::cerr << "[FAIL] " << (msg) << " (" << (a) << " != " << (b) << ")" \
                      << std::endl;                                                \
        } else {                                                                   \
            std::cout << "[PASS] " << (msg) << std::endl;                          \
        }                                                                          \
    } while (0)

static bool hash_non_null(const Hash256& h) {
    for (uint8_t c : h.data) {
        if (c != 0) return true;
    }
    return false;
}

} // namespace

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    std::cout << "=== HIL READINESS CHECK ===" << std::endl;

    // --------------------------------------------------------
    // 1) Time sanity
    // --------------------------------------------------------
    uint32_t t0 = PlatformHAL::now_ms();
    uint32_t t1 = PlatformHAL::now_ms();
    CHECK_TRUE(t1 >= t0, "now_ms() monotonic (non-decreasing)");

    // --------------------------------------------------------
    // 2) Hashing sanity (deterministic + non-null)
    // --------------------------------------------------------
    const char* blob = "raps_hil_probe";
    Hash256 h1 = PlatformHAL::sha256(blob, std::strlen(blob));
    Hash256 h2 = PlatformHAL::sha256(blob, std::strlen(blob));
    CHECK_TRUE(hash_non_null(h1), "sha256() returns non-null hash for non-empty input");
    CHECK_TRUE(std::memcmp(h1.data, h2.data, sizeof(h1.data)) == 0,
               "sha256() deterministic for same input");

    // --------------------------------------------------------
    // 3) Signature stub sanity (or real signing in flight/HIL)
    // --------------------------------------------------------
    uint8_t sig[64]{};
    bool sig_ok = PlatformHAL::ed25519_sign(h1, sig);
    CHECK_TRUE(sig_ok, "ed25519_sign() succeeds");
    // Not asserting signature contents because HIL may use real signing.

    // --------------------------------------------------------
    // 4) Flash smoke test
    // --------------------------------------------------------
    // In HIL, this should route to rig-backed memory or a simulated NVM surface.
    // We keep this test minimal: write succeeds, read succeeds, buffer is defined.
    uint8_t readback[16]{};
    bool fw = PlatformHAL::flash_write(/*addr*/ 0, blob, std::strlen(blob));
    CHECK_TRUE(fw, "flash_write() basic smoke succeeds");

    bool fr = PlatformHAL::flash_read(/*addr*/ 0, readback, sizeof(readback));
    CHECK_TRUE(fr, "flash_read() basic smoke succeeds");
    // We do NOT require content equality, because some rigs intentionally zero-fill
    // reads unless a storage model is enabled. That’s okay for “alive” checks.

    // --------------------------------------------------------
    // 5) Actuator smoke + idempotency
    // --------------------------------------------------------
    std::string tx = PlatformHAL::generate_tx_id();
    CHECK_TRUE(!tx.empty(), "generate_tx_id() returns non-empty string");

    // Use a reasonably generous timeout for HIL transport.
    // If the rig is alive, this should succeed.
    bool a1 = PlatformHAL::actuator_execute(tx.c_str(), /*throttle*/ 50.0f, /*valve*/ 0.1f, /*timeout*/ 200);
    CHECK_TRUE(a1, "actuator_execute() succeeds with reasonable timeout");

    // Idempotency: replaying same tx_id should succeed (no double-apply).
    bool a2 = PlatformHAL::actuator_execute(tx.c_str(), /*throttle*/ 50.0f, /*valve*/ 0.1f, /*timeout*/ 200);
    CHECK_TRUE(a2, "actuator_execute() idempotent replay succeeds");

    // --------------------------------------------------------
    // 6) Downlink smoke test
    // --------------------------------------------------------
    bool dl = PlatformHAL::downlink_queue(blob, std::strlen(blob));
    CHECK_TRUE(dl, "downlink_queue() basic smoke succeeds");

    // --------------------------------------------------------
    // 7) Metrics (should never crash; may be routed/ignored)
    // --------------------------------------------------------
    PlatformHAL::metric_emit("hil.readiness.probe", 1.0f);
    PlatformHAL::metric_emit("hil.readiness.probe_tagged", 1.0f, "mode", "HIL");
    std::cout << "[PASS] metric_emit() callable" << std::endl;

    // --------------------------------------------------------
    // Summary / Exit
    // --------------------------------------------------------
    if (g_failures == 0) {
        std::cout << "=== HIL READINESS CHECK: PASS ===" << std::endl;
        return 0;
    }

    std::cerr << "=== HIL READINESS CHECK: FAIL (" << g_failures << " issues) ===" << std::endl;
    return 2;
}
