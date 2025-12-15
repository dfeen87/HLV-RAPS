#include "platform/platform_hal.hpp"

#include <chrono>
#include <random>
#include <cstring>

#ifdef RAPS_ENABLE_SIL_SINK
#include "tests/sil/sil_metric_sink.hpp"
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
        acc = (acc * 131) ^ p[i];
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
// Flash (STUB)
// -----------------------------------------------------------------------------
bool PlatformHAL::flash_write(uint32_t, const void*, size_t) {
    return true;
}

bool PlatformHAL::flash_read(uint32_t, void* data, size_t len) {
    std::memset(data, 0, len);
    return true;
}

// -----------------------------------------------------------------------------
// Actuator (STUB)
// -----------------------------------------------------------------------------
bool PlatformHAL::actuator_execute(
    const char*,
    float,
    float,
    uint32_t
) {
    return true;
}

// -----------------------------------------------------------------------------
// Telemetry downlink (STUB)
// -----------------------------------------------------------------------------
bool PlatformHAL::downlink_queue(const void*, size_t) {
    return true;
}

// -----------------------------------------------------------------------------
// Metrics — THIS IS THE IMPORTANT PART
// -----------------------------------------------------------------------------
void PlatformHAL::metric_emit(const char* name, float value) {
#ifdef RAPS_ENABLE_SIL_SINK
    SilMetricSink::emit(name, value);
#else
    (void)name;
    (void)value;
#endif
}

void PlatformHAL::metric_emit(
    const char* name,
    float value,
    const char* tag_key,
    const char* tag_value
) {
#ifdef RAPS_ENABLE_SIL_SINK
    SilMetricSink::emit(name, value, tag_key, tag_value);
#else
    (void)name;
    (void)value;
    (void)tag_key;
    (void)tag_value;
#endif
}

// -----------------------------------------------------------------------------
// RNG helpers (non-crypto)
// -----------------------------------------------------------------------------
static std::mt19937 g_rng{12345};

void PlatformHAL::seed_rng_for_stubs(uint32_t seed) {
    g_rng.seed(seed);
}

float PlatformHAL::random_float(float min, float max) {
    std::uniform_real_distribution<float> d(min, max);
    return d(g_rng);
}

std::string PlatformHAL::generate_tx_id() {
    static const char* hex = "0123456789abcdef";
    std::string out;
    out.reserve(24);
    for (int i = 0; i < 24; ++i) {
        out.push_back(hex[g_rng() % 16]);
    }
    return out;
}
