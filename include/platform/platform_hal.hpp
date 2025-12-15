#pragma once

#include <cstdint>
#include <cstddef>
#include <string>
#include <random>

#include "raps/core/raps_definitions.hpp"

// ============================================================================
// PlatformHAL
// - Target-agnostic hardware abstraction (SIL-friendly stubs live in .cpp)
// - Flight builds can replace the .cpp implementation entirely
// ============================================================================

class PlatformHAL {
public:
    // ------------------------------------------------------------------------
    // Time
    // ------------------------------------------------------------------------
    static uint32_t now_ms();

    // ------------------------------------------------------------------------
    // Crypto (stub in SIL; flight replaces with certified/HSM-backed impl)
    // ------------------------------------------------------------------------
    static Hash256 sha256(const void* data, size_t len);
    static bool ed25519_sign(const Hash256& msg, uint8_t signature[64]);

    // ------------------------------------------------------------------------
    // Storage (stub in SIL; flight uses redundant flash drivers)
    // ------------------------------------------------------------------------
    static bool flash_write(uint32_t address, const void* data, size_t len);
    static bool flash_read(uint32_t address, void* data, size_t len);

    // ------------------------------------------------------------------------
    // Actuation (stub in SIL; flight uses transaction-aware nonblocking driver)
    // ------------------------------------------------------------------------
    static bool actuator_execute(
        const char* tx_id,
        float throttle,
        float valve,
        uint32_t timeout_ms
    );

    // ------------------------------------------------------------------------
    // Telemetry
    // ------------------------------------------------------------------------
    static bool downlink_queue(const void* data, size_t len);

    // ------------------------------------------------------------------------
    // Metrics
    // - In SIL: can be routed to a sink; in production: to telemetry
    // ------------------------------------------------------------------------
    static void metric_emit(const char* name, float value);
    static void metric_emit(
        const char* name,
        float value,
        const char* tag_key,
        const char* tag_value
    );

    // ------------------------------------------------------------------------
    // RNG helpers (ONLY for stubs/tests; never for crypto)
    // ------------------------------------------------------------------------
    static void seed_rng_for_stubs(uint32_t seed);
    static float random_float(float min, float max);
    static std::string generate_tx_id();

#ifdef RAPS_ENABLE_SIL_FAULTS
    // ------------------------------------------------------------------------
    // SIL Fault Injection (compile-time gated)
    // - Never compiled into flight unless explicitly enabled
    // ------------------------------------------------------------------------
    struct SilFaultConfig {
        // One-shot faults
        bool flash_write_fail_once = false;
        bool actuator_timeout_once = false;

        // Probabilistic faults (0.0..1.0)
        float flash_write_fail_probability = 0.0f;
        float actuator_timeout_probability = 0.0f;

        // Forced simulated actuator latency (ms). If >= 0, overrides.
        int32_t actuator_forced_latency_ms = -1;
    };

    static void sil_set_fault_config(const SilFaultConfig& cfg);
    static SilFaultConfig sil_get_fault_config();
    static void sil_reset_faults();
#endif

private:
    static std::mt19937_64 rng_;
    static bool rng_seeded_;
};
