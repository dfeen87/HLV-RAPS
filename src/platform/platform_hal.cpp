// ============================================================
// PlatformHAL — Host/SIL implementation
// - Deterministic-friendly RNG stubs
// - Fault injection hooks (compile-time gated)
// - Idempotent actuator execution
// - CI-friendly, no external deps
// ============================================================

#include "platform/platform_hal.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <iostream>   // <-- added (optional metrics)
#include <mutex>
#include <random>
#include <sstream>
#include <string>
#include <unordered_set>

// ------------------------------------------------------------
// Optional metric printing (OFF by default)
// Enable with: -DRAPS_PLATFORM_METRICS_STDOUT=1
// ------------------------------------------------------------
#ifndef RAPS_PLATFORM_METRICS_STDOUT
#define RAPS_PLATFORM_METRICS_STDOUT 0
#endif

// ------------------------------------------------------------
// Static members
// ------------------------------------------------------------

std::mt19937_64 PlatformHAL::rng_;
bool PlatformHAL::rng_seeded_ = false;

// ------------------------------------------------------------
// Internal helpers (SIL fault injection)
// ------------------------------------------------------------

namespace {

struct FaultConfig {
    // Probabilities in [0, 1]
    float flash_write_fail_prob = 0.005f;   // 0.5%
    float flash_read_fail_prob  = 0.001f;   // 0.1%
    float downlink_fail_prob    = 0.001f;   // 0.1%
    float actuator_fail_prob    = 0.002f;   // 0.2%

    // Latency model for actuator_execute
    float actuator_latency_min_s = 0.003f;
    float actuator_latency_max_s = 0.020f;
};

FaultConfig& fault_cfg() {
    static FaultConfig cfg{};
    return cfg;
}

std::mutex& hal_mutex() {
    static std::mutex m;
    return m;
}

std::unordered_set<std::string>& applied_tx_ids() {
    static std::unordered_set<std::string> s;
    return s;
}

// forces seed in random_float()
bool rng_ready() {
    return PlatformHAL::random_float(0.0f, 1.0f) >= 0.0f;
}

bool should_fail(float prob) {
#if defined(RAPS_ENABLE_SIL_FAULTS) && (RAPS_ENABLE_SIL_FAULTS == 1)
    if (prob <= 0.0f) return false;
    if (prob >= 1.0f) return true;
    float r = PlatformHAL::random_float(0.0f, 1.0f);
    return r < prob;
#else
    (void)prob;
    return false;
#endif
}

} // namespace

// ------------------------------------------------------------
// Time
// ------------------------------------------------------------

uint32_t PlatformHAL::now_ms() {
    // Monotonic time — good for SIL/host tests.
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()
    ).count();

    // uint32 wrap is acceptable for monotonic deltas in embedded-style code.
    return static_cast<uint32_t>(ms);
}

// ------------------------------------------------------------
// Crypto (STUBS — NOT production crypto)
// ------------------------------------------------------------

Hash256 PlatformHAL::sha256(const void* data, size_t len) {
    Hash256 h{};
    std::memset(h.data, 0, sizeof(h.data));

    if (!data || len == 0) {
        return h;
    }

    // Non-cryptographic placeholder for SIL.
    // Deterministic: sums bytes + mixes length.
    uint64_t sum = 0;
    const uint8_t* b = static_cast<const uint8_t*>(data);
    for (size_t i = 0; i < len; ++i) {
        sum = (sum * 1315423911u) ^ b[i] ^ (sum >> 16);
    }

    // Copy some mixed bytes
    std::memcpy(h.data, &sum, std::min(sizeof(sum), sizeof(h.data)));
    h.data[8]  = static_cast<uint8_t>(len & 0xFF);
    h.data[9]  = static_cast<uint8_t>((len >> 8) & 0xFF);
    h.data[10] = static_cast<uint8_t>((len >> 16) & 0xFF);
    h.data[11] = static_cast<uint8_t>((len >> 24) & 0xFF);

    // Fill remainder deterministically
    for (size_t i = 12; i < 32; ++i) {
        h.data[i] = static_cast<uint8_t>((sum >> ((i % 8) * 8)) & 0xFF)
                  ^ static_cast<uint8_t>(i * 17);
    }

    return h;
}

bool PlatformHAL::ed25519_sign(const Hash256& /*msg*/, uint8_t signature[64]) {
    if (!signature) return false;
    // Stub signature pattern for SIL
    std::memset(signature, 0xAB, 64);
    return true;
}

// ------------------------------------------------------------
// Flash (STUBS)
// ------------------------------------------------------------

bool PlatformHAL::flash_write(uint32_t /*address*/, const void* /*data*/, size_t /*len*/) {
    // Optional SIL fault injection
    if (should_fail(fault_cfg().flash_write_fail_prob)) {
        return false;
    }
    return true;
}

bool PlatformHAL::flash_read(uint32_t /*address*/, void* data, size_t len) {
    if (!data) return false;

    if (should_fail(fault_cfg().flash_read_fail_prob)) {
        return false;
    }

    // Stub: returns zeroed bytes
    std::memset(data, 0, len);
    return true;
}

// ------------------------------------------------------------
// Actuator interface (STUB)
// - Idempotent by tx_id
// - Simulated latency
// - Optional SIL failures
// ------------------------------------------------------------

bool PlatformHAL::actuator_execute(
    const char* tx_id,
    float /*throttle*/,
    float /*valve*/,
    uint32_t timeout_ms
) {
    if (!tx_id || tx_id[0] == '\0') {
        return false;
    }

    // Ensure RNG seeded (deterministic tests can seed explicitly via seed_rng_for_stubs)
    (void)rng_ready();

    // Idempotency: if we've already applied this tx_id, succeed immediately.
    {
        std::lock_guard<std::mutex> lk(hal_mutex());
        auto& set = applied_tx_ids();
        if (set.find(tx_id) != set.end()) {
            // In a real system, this means "already applied".
            return true;
        }
    }

    // Simulate latency
    std::uniform_real_distribution<float> d(
        fault_cfg().actuator_latency_min_s,
        fault_cfg().actuator_latency_max_s
    );

    float latency_s = d(rng_);
    uint32_t latency_ms = static_cast<uint32_t>(latency_s * 1000.0f);

    if (latency_ms > timeout_ms) {
        return false; // timeout
    }

#if defined(RAPS_ENABLE_SIL_FAULTS) && (RAPS_ENABLE_SIL_FAULTS == 1)
    if (should_fail(fault_cfg().actuator_fail_prob)) {
        return false;
    }
#endif

    // Mark tx applied (idempotency guarantee)
    {
        std::lock_guard<std::mutex> lk(hal_mutex());
        applied_tx_ids().insert(std::string(tx_id));
    }

    return true;
}

// ------------------------------------------------------------
// Downlink queue (STUB)
// ------------------------------------------------------------

bool PlatformHAL::downlink_queue(const void* /*data*/, size_t /*len*/) {
    if (should_fail(fault_cfg().downlink_fail_prob)) {
        return false;
    }
    return true;
}

// ------------------------------------------------------------
// Metrics (STUB by default; opt-in stdout)
// ------------------------------------------------------------

void PlatformHAL::metric_emit(const char* name, float value) {
#if RAPS_PLATFORM_METRICS_STDOUT
    if (!name) name = "metric.null";
    std::cout << "[METRIC] " << name << "=" << value << "\n";
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
#if RAPS_PLATFORM_METRICS_STDOUT
    if (!name) name = "metric.null";
    if (!tag_key) tag_key = "tag";
    if (!tag_value) tag_value = "null";
    std::cout << "[METRIC] " << name << "=" << value << " " << tag_key << "=" << tag_value << "\n";
#else
    (void)name;
    (void)value;
    (void)tag_key;
    (void)tag_value;
#endif
}

// ------------------------------------------------------------
// RNG for stubs (NOT crypto)
// ------------------------------------------------------------

void PlatformHAL::seed_rng_for_stubs(uint32_t seed) {
    rng_.seed(static_cast<uint64_t>(seed));
    rng_seeded_ = true;

    // Reset idempotency history for deterministic tests when reseeding
    std::lock_guard<std::mutex> lk(hal_mutex());
    applied_tx_ids().clear();
}

float PlatformHAL::random_float(float min, float max) {
    if (!rng_seeded_) {
        // Default seed so behavior is stable-ish even if caller forgets to seed.
        seed_rng_for_stubs(1);
    }

    if (max < min) std::swap(min, max);
    std::uniform_real_distribution<float> dist(min, max);
    return dist(rng_);
}

std::string PlatformHAL::generate_tx_id() {
    if (!rng_seeded_) {
        seed_rng_for_stubs(1);
    }

    static constexpr char HEX[] = "0123456789abcdef";
    std::uniform_int_distribution<int> dist(0, 15);

    std::string s;
    s.reserve(24);
    for (int i = 0; i < 24; ++i) {
        s.push_back(HEX[dist(rng_)]);
    }
    return s;
}
