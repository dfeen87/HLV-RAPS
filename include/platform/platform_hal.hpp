#pragma once

#include <cstdint>
#include <cstddef>
#include <string>
#include <random>

#include "raps/core/raps_definitions.hpp"

// SIL configuration is header-only and compile-time gated
#include "raps/platform/sil_fault_injection_config.hpp"

// ============================================================================
// PlatformHAL
// ---------------------------------------------------------------------------
// Target-agnostic hardware abstraction layer
//
// Design goals:
//  • SIL-safe deterministic behavior
//  • Compile-time gated fault injection
//  • Flight builds may replace .cpp entirely
//  • Zero dynamic allocation requirements
//
// NOTE:
//  - This interface is STABLE.
//  - Flight-certified implementations override the .cpp, not this header.
// ============================================================================

class PlatformHAL {
public:
    // ------------------------------------------------------------------------
    // Time
    // ------------------------------------------------------------------------
    // Monotonic timestamp in milliseconds.
    // Wraparound is acceptable; callers must use deltas.
    static uint32_t now_ms();

    // ------------------------------------------------------------------------
    // Crypto (SIL stub; flight replaces with certified/HSM-backed impl)
    // ------------------------------------------------------------------------
    static Hash256 sha256(const void* data, size_t len);
    static bool ed25519_sign(const Hash256& msg, uint8_t signature[64]);

    // ------------------------------------------------------------------------
    // Persistent storage (SIL stub; flight uses redundant flash drivers)
    // ------------------------------------------------------------------------
    static bool flash_write(uint32_t address, const void* data, size_t len);
    static bool flash_read(uint32_t address, void* data, size_t len);

    // ------------------------------------------------------------------------
    // Actuation (idempotent, transaction-aware)
    // ------------------------------------------------------------------------
    static bool actuator_execute(
        const char* tx_id,
        float throttle,
        float valve,
        uint32_t timeout_ms
    );

    // ------------------------------------------------------------------------
    // Telemetry / downlink
    // ------------------------------------------------------------------------
    static bool downlink_queue(const void* data, size_t len);

    // ------------------------------------------------------------------------
    // Metrics
    // ------------------------------------------------------------------------
    // In SIL: no-op or redirected to test sink
    // In flight: forwarded to telemetry / health monitor
    static void metric_emit(const char* name, float value);
    static void metric_emit(
        const char* name,
        float value,
        const char* tag_key,
        const char* tag_value
    );

    // ------------------------------------------------------------------------
    // RNG helpers (STRICTLY for SIL / tests — NEVER crypto)
    // ------------------------------------------------------------------------
    static void seed_rng_for_stubs(uint32_t seed);
    static float random_float(float min, float max);
    static std::string generate_tx_id();

#if RAPS_ENABLE_SIL_FAULTS
    // ------------------------------------------------------------------------
    // SIL Fault Injection Controls
    // ------------------------------------------------------------------------
    // These APIs are:
    //  • compile-time gated
    //  • never linked into flight builds unless explicitly enabled
    //  • safe for CI, fuzzing, and coverage stress tests
    //
    // Deterministic-first: probabilities are optional.
    // ------------------------------------------------------------------------

    struct SilFaultConfig {
        // One-shot deterministic faults
        bool flash_write_fail_once = false;
        bool actuator_timeout_once = false;

        // Probabilistic faults (0.0f – 1.0f)
        float flash_write_fail_probability = 0.0f;
        float actuator_timeout_probability = 0.0f;

        // Forced actuator latency override (ms)
        // < 0 → disabled
        int32_t actuator_forced_latency_ms = -1;
    };

    static void sil_set_fault_config(const SilFaultConfig& cfg);
    static SilFaultConfig sil_get_fault_config();
    static void sil_reset_faults();
#endif

private:
    // Shared RNG for SIL-only stubs
    static std::mt19937_64 rng_;
    static bool rng_seeded_;
};
