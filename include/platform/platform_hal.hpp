#pragma once

#include <cstdint>
#include <cstddef>
#include <string>
#include <random>

#include "raps_definitions.hpp"

// =====================================================
// Platform Abstraction Layer (HAL)
// =====================================================
//
// Hardware / RTOS abstraction boundary.
// All methods are static to reflect global system resources.
// In flight builds, these would bind to certified drivers.
//

class PlatformHAL {
public:
    // Monotonic millisecond timestamp
    static uint32_t now_ms();

    // Cryptographic primitives (stubbed for demo)
    static Hash256 sha256(const void* data, size_t len);
    static bool ed25519_sign(const Hash256& msg, uint8_t signature[64]);

    // Persistent storage interface
    static bool flash_write(uint32_t address, const void* data, size_t len);
    static bool flash_read(uint32_t address, void* data, size_t len);

    // Actuator interface (transaction-aware, non-blocking)
    static bool actuator_execute(
        const char* tx_id,
        float thrust_kN,
        float gimbal_angle_rad,
        uint32_t timeout_ms
    );

    // Telemetry / downlink interface
    static bool downlink_queue(const void* data, size_t len);

    // Metric emission
    static void metric_emit(const char* name, float value);
    static void metric_emit(
        const char* name,
        float value,
        const char* tag_key,
        const char* tag_value
    );

    // RNG helpers (for stubs only)
    static void seed_rng_for_stubs(uint32_t seed);
    static float random_float(float min, float max);

    // Transaction ID generator
    static std::string generate_tx_id();

private:
    static std::mt19937_64 rng_;
    static bool rng_seeded_;
};
