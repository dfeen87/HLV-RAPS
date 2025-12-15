#include "platform/platform_hal.hpp"

#include <chrono>
#include <random>
#include <cstring>
#include <algorithm>

#ifdef RAPS_ENABLE_SIL_SINK
#include "tests/sil/sil_metric_sink.hpp"
#endif

// Static members
std::mt19937_64 PlatformHAL::rng_{12345};
bool PlatformHAL::rng_seeded_ = false;

#ifdef RAPS_ENABLE_SIL_FAULTS
static PlatformHAL::SilFaultConfig g_sil_faults{};
#endif

// -----------------------------------------------------------------------------
// Time source
// -----------------------------------------------------------------------------
uint32_t PlatformHAL::now_ms() {
    using namespace std::chrono;
    return static_cast<uint32_t>(
        duration_cast<milliseconds>(
            steady_clock::now().time_since_epoch()
        ).count()
    );
}

// -----------------------------------------------------------------------------
// Crypto (STUBS — deterministic, non-flight)
// -----------------------------------------------------------------------------
Hash256 PlatformHAL::sha256(const void* data, size_t len) {
    Hash256 h{};
    if (!data || len == 0) return h;

    const uint8_t* p = static_cast<const uint8_t*>(data);
    uint32_t acc = 0;
    for (size_t i = 0; i < len; ++i) {
        acc = (acc * 131u) ^ p[i];
    }
    for (size_t i = 0; i < 32; ++i) {
        h.data[i] = static_cast<uint8_t>((acc >> ((i % 4) * 8)) & 0xFF);
    }
    return h;
}

bool PlatformHAL::ed25519_sign(const Hash256&, uint8_t signature[64]) {
    std::memset(signature, 0, 64);
    return true;
}

// -----------------------------------------------------------------------------
// Flash/persistent storage (STUB + SIL faults)
// -----------------------------------------------------------------------------
bool PlatformHAL::flash_write(uint32_t, const void*, size_t) {
#ifdef RAPS_ENABLE_SIL_FAULTS
    if (g_sil_faults.flash_write_fail_once) {
        g_sil_faults.flash_write_fail_once = false;
#ifdef RAPS_ENABLE_SIL_SINK
        SilMetricSink::emit("sil.fault.flash_write_fail_once", 1.0f);
#endif
        return false;
    }
    if (g_sil_faults.flash_write_fail_probability > 0.0f) {
        std::uniform_real_distribution<float> d(0.0f, 1.0f);
        if (d(rng_) < g_sil_faults.flash_write_fail_probability) {
#ifdef RAPS_ENABLE_SIL_SINK
            SilMetricSink::emit("sil.fault.flash_write_fail_prob", 1.0f);
#endif
            return false;
        }
    }
#endif
    return true;
}

bool PlatformHAL::flash_read(uint32_t, void* data, size_t len) {
    std::memset(data, 0, len);
    return true;
}

// -----------------------------------------------------------------------------
// Actuator interface (STUB + SIL faults)
// -----------------------------------------------------------------------------
bool PlatformHAL::actuator_execute(const char*, float, float, uint32_t timeout_ms) {
#ifdef RAPS_ENABLE_SIL_FAULTS
    if (g_sil_faults.actuator_timeout_once) {
        g_sil_faults.actuator_timeout_once = false;
#ifdef RAPS_ENABLE_SIL_SINK
        SilMetricSink::emit("sil.fault.actuator_timeout_once", 1.0f);
#endif
        return false;
    }

    if (g_sil_faults.actuator_forced_latency_ms >= 0) {
        return static_cast<uint32_t>(g_sil_faults.actuator_forced_latency_ms) <= timeout_ms;
    }

    if (g_sil_faults.actuator_timeout_probability > 0.0f) {
        std::uniform_real_distribution<float> d(0.0f, 1.0f);
        if (d(rng_) < g_sil_faults.actuator_timeout_probability) {
#ifdef RAPS_ENABLE_SIL_SINK
            SilMetricSink::emit("sil.fault.actuator_timeout_prob", 1.0f);
#endif
            return false;
        }
    }
#endif

    // Nominal SIL behavior: succeed.
    (void)timeout_ms;
    return true;
}

// -----------------------------------------------------------------------------
// Telemetry downlink queue (STUB)
// -----------------------------------------------------------------------------
bool PlatformHAL::downlink_queue(const void*, size_t) {
    return true;
}

// -----------------------------------------------------------------------------
// Metrics — route into SIL sink when enabled
// -----------------------------------------------------------------------------
void PlatformHAL::metric_emit(const char* name, float value) {
#ifdef RAPS_ENABLE_SIL_SINK
    SilMetricSink::emit(name, value);
#else
    (void)name; (void)value;
#endif
}

void PlatformHAL::metric_emit(const char* name, float value, const char* tag_key, const char* tag_value) {
#ifdef RAPS_ENABLE_SIL_SINK
    SilMetricSink::emit(name, value, tag_key, tag_value);
#else
    (void)name; (void)value; (void)tag_key; (void)tag_value;
#endif
}

// -----------------------------------------------------------------------------
// RNG helpers (non-crypto)
// -----------------------------------------------------------------------------
void PlatformHAL::seed_rng_for_stubs(uint32_t seed) {
    rng_.seed(seed);
    rng_seeded_ = true;
}

float PlatformHAL::random_float(float min, float max) {
    if (!rng_seeded_) seed_rng_for_stubs(1);
    std::uniform_real_distribution<float> d(min, max);
    return d(rng_);
}

std::string PlatformHAL::generate_tx_id() {
    if (!rng_seeded_) seed_rng_for_stubs(1);
    static const char* hex = "0123456789abcdef";
    std::string out;
    out.reserve(24);
    for (int i = 0; i < 24; ++i) out.push_back(hex[rng_() % 16]);
    return out;
}

#ifdef RAPS_ENABLE_SIL_FAULTS
void PlatformHAL::sil_set_fault_config(const SilFaultConfig& cfg) {
    g_sil_faults = cfg;
#ifdef RAPS_ENABLE_SIL_SINK
    SilMetricSink::emit("sil.fault.config_set", 1.0f);
#endif
}

PlatformHAL::SilFaultConfig PlatformHAL::sil_get_fault_config() {
    return g_sil_faults;
}

void PlatformHAL::sil_reset_faults() {
    g_sil_faults = SilFaultConfig{};
#ifdef RAPS_ENABLE_SIL_SINK
    SilMetricSink::emit("sil.fault.reset", 1.0f);
#endif
}
#endif
