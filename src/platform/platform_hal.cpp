#include "platform/platform_hal.hpp"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <iomanip>
#include <mutex>
#include <random>
#include <sstream>
#include <string>

// Static members
std::mt19937_64 PlatformHAL::rng_;
bool PlatformHAL::rng_seeded_ = false;

#ifdef RAPS_ENABLE_SIL_FAULTS
namespace {
std::mutex g_sil_fault_mutex;
PlatformHAL::SilFaultConfig g_sil_fault_cfg{};
}
#endif

// -----------------------------------------------------------------------------
// Time
// -----------------------------------------------------------------------------
uint32_t PlatformHAL::now_ms() {
    using namespace std::chrono;
    return static_cast<uint32_t>(
        duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count()
    );
}

// -----------------------------------------------------------------------------
// Crypto (SIL stub; NOT cryptographically secure)
// -----------------------------------------------------------------------------
Hash256 PlatformHAL::sha256(const void* data, size_t len) {
    Hash256 h = Hash256::null_hash();
    if (!data || len == 0) return h;

    // Simple, deterministic mixing (NOT real SHA-256).
    // Goal: stable IDs for SIL + tests.
    const uint8_t* p = static_cast<const uint8_t*>(data);

    uint64_t a = 1469598103934665603ULL; // FNV-like seed
    uint64_t b = 1099511628211ULL;
    for (size_t i = 0; i < len; ++i) {
        a ^= static_cast<uint64_t>(p[i]);
        a *= b;
        a ^= (a >> 33);
        a *= 0xff51afd7ed558ccdULL;
    }

    // Expand into 32 bytes
    for (size_t i = 0; i < 32; i += 8) {
        uint64_t v = a + (0x9e3779b97f4a7c15ULL * (i + 1)) + (static_cast<uint64_t>(len) << 1);
        std::memcpy(&h.data[i], &v, 8);
        a ^= (v >> 29);
        a *= 0xc4ceb9fe1a85ec53ULL;
    }

    return h;
}

bool PlatformHAL::ed25519_sign(const Hash256& /*msg*/, uint8_t signature[64]) {
    // SIL stub: fills deterministic-ish pattern
    if (!signature) return false;
    std::memset(signature, 0xAB, 64);
    return true;
}

// -----------------------------------------------------------------------------
// RNG helpers (ONLY for stubs/tests; never for crypto)
// -----------------------------------------------------------------------------
void PlatformHAL::seed_rng_for_stubs(uint32_t seed) {
    rng_.seed(static_cast<uint64_t>(seed));
    rng_seeded_ = true;
}

float PlatformHAL::random_float(float min, float max) {
    if (!rng_seeded_) seed_rng_for_stubs(1);
    std::uniform_real_distribution<float> dist(min, max);
    return dist(rng_);
}

std::string PlatformHAL::generate_tx_id() {
    if (!rng_seeded_) seed_rng_for_stubs(1);

    static constexpr char kHex[] = "0123456789abcdef";
    std::string out;
    out.reserve(24);

    std::uniform_int_distribution<int> dist(0, 15);
    for (int i = 0; i < 24; ++i) {
        out.push_back(kHex[dist(rng_)]);
    }
    return out;
}

// -----------------------------------------------------------------------------
// Storage (SIL stub)
// -----------------------------------------------------------------------------
bool PlatformHAL::flash_write(uint32_t /*address*/, const void* /*data*/, size_t /*len*/) {
    if (!rng_seeded_) seed_rng_for_stubs(1);

#ifdef RAPS_ENABLE_SIL_FAULTS
    {
        std::lock_guard<std::mutex> lk(g_sil_fault_mutex);

        if (g_sil_fault_cfg.flash_write_fail_once) {
            g_sil_fault_cfg.flash_write_fail_once = false;
            metric_emit("sil.fault.flash_write_fail_once", 1.0f);
            return false;
        }

        if (g_sil_fault_cfg.flash_write_fail_probability > 0.0f) {
            std::uniform_real_distribution<float> u(0.0f, 1.0f);
            if (u(rng_) < g_sil_fault_cfg.flash_write_fail_probability) {
                metric_emit("sil.fault.flash_write_fail_probability", 1.0f);
                return false;
            }
        }
    }
#endif

    // Default SIL behavior: succeed
    return true;
}

bool PlatformHAL::flash_read(uint32_t /*address*/, void* data, size_t len) {
    if (!data || len == 0) return false;

    // Default SIL behavior: return zeroed data
    std::memset(data, 0, len);
    return true;
}

// -----------------------------------------------------------------------------
// Actuation (SIL stub)
// -----------------------------------------------------------------------------
bool PlatformHAL::actuator_execute(
    const char* /*tx_id*/,
    float /*throttle*/,
    float /*valve*/,
    uint32_t timeout_ms
) {
    if (!rng_seeded_) seed_rng_for_stubs(1);

    int32_t simulated_latency_ms = 0;

#ifdef RAPS_ENABLE_SIL_FAULTS
    {
        std::lock_guard<std::mutex> lk(g_sil_fault_mutex);

        // Forced latency override
        if (g_sil_fault_cfg.actuator_forced_latency_ms >= 0) {
            simulated_latency_ms = g_sil_fault_cfg.actuator_forced_latency_ms;
        } else {
            std::uniform_int_distribution<int32_t> d(3, 20);
            simulated_latency_ms = d(rng_);
        }

        // One-shot timeout fault
        if (g_sil_fault_cfg.actuator_timeout_once) {
            g_sil_fault_cfg.actuator_timeout_once = false;
            metric_emit("sil.fault.actuator_timeout_once", 1.0f);
            simulated_latency_ms = static_cast<int32_t>(timeout_ms) + 1;
        }

        // Probabilistic timeout fault
        if (g_sil_fault_cfg.actuator_timeout_probability > 0.0f) {
            std::uniform_real_distribution<float> u(0.0f, 1.0f);
            if (u(rng_) < g_sil_fault_cfg.actuator_timeout_probability) {
                metric_emit("sil.fault.actuator_timeout_probability", 1.0f);
                simulated_latency_ms = static_cast<int32_t>(timeout_ms) + 1;
            }
        }
    }
#else
    // No faults compiled: normal latency
    std::uniform_int_distribution<int32_t> d(3, 20);
    simulated_latency_ms = d(rng_);
#endif

    // Simulated timeout behavior
    if (static_cast<uint32_t>(std::max<int32_t>(0, simulated_latency_ms)) > timeout_ms) {
        metric_emit("actuator.timeout", 1.0f);
        return false;
    }

    return true;
}

// -----------------------------------------------------------------------------
// Telemetry / Metrics
// -----------------------------------------------------------------------------
bool PlatformHAL::downlink_queue(const void* /*data*/, size_t /*len*/) {
    // SIL stub: always succeeds
    return true;
}

void PlatformHAL::metric_emit(const char* /*name*/, float /*value*/) {
    // Default: no-op (keeps SIL quiet)
}

void PlatformHAL::metric_emit(
    const char* /*name*/,
    float /*value*/,
    const char* /*tag_key*/,
    const char* /*tag_value*/
) {
    // Default: no-op (keeps SIL quiet)
}

#ifdef RAPS_ENABLE_SIL_FAULTS
// -----------------------------------------------------------------------------
// SIL Fault Injection API
// -----------------------------------------------------------------------------
void PlatformHAL::sil_set_fault_config(const SilFaultConfig& cfg) {
    std::lock_guard<std::mutex> lk(g_sil_fault_mutex);
    g_sil_fault_cfg = cfg;
    metric_emit("sil.fault.config_set", 1.0f);
}

PlatformHAL::SilFaultConfig PlatformHAL::sil_get_fault_config() {
    std::lock_guard<std::mutex> lk(g_sil_fault_mutex);
    return g_sil_fault_cfg;
}

void PlatformHAL::sil_reset_faults() {
    std::lock_guard<std::mutex> lk(g_sil_fault_mutex);
    g_sil_fault_cfg = SilFaultConfig{};
    metric_emit("sil.fault.reset", 1.0f);
}
#endif
