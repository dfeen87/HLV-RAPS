// ============================================================
// PlatformHAL — HIL implementation
// - Delegates IO to an injected HilDeviceInterface
// - Keeps the rest of RAPS unchanged
// ============================================================

#include "platform/platform_hal.hpp"
#include "raps/hil/hil_device_interface.hpp"

#include <cstring>
#include <string>

namespace {

HilDeviceInterface* dev_or_null() {
    return hil_get_device();
}

HilDeviceInterface* dev_or_fail(const char* what) {
    HilDeviceInterface* d = dev_or_null();
    if (!d) {
        // Fail-closed behavior: if HIL device isn’t set, we do not pretend IO worked.
        // NOTE: no exceptions in flight-style code; just hard false.
        (void)what;
    }
    return d;
}

} // namespace

uint32_t PlatformHAL::now_ms() {
    if (auto* d = dev_or_fail("now_ms")) return d->now_ms();
    return 0;
}

Hash256 PlatformHAL::sha256(const void* data, size_t len) {
    Hash256 h{};
    std::memset(h.data, 0, sizeof(h.data));
    if (auto* d = dev_or_fail("sha256")) return d->sha256(data, len);
    return h;
}

bool PlatformHAL::ed25519_sign(const Hash256& msg, uint8_t signature[64]) {
    if (!signature) return false;
    if (auto* d = dev_or_fail("ed25519_sign")) return d->ed25519_sign(msg, signature);
    std::memset(signature, 0, 64);
    return false;
}

bool PlatformHAL::flash_write(uint32_t address, const void* data, size_t len) {
    if (auto* d = dev_or_fail("flash_write")) return d->flash_write(address, data, len);
    return false;
}

bool PlatformHAL::flash_read(uint32_t address, void* data, size_t len) {
    if (auto* d = dev_or_fail("flash_read")) return d->flash_read(address, data, len);
    return false;
}

bool PlatformHAL::actuator_execute(
    const char* tx_id,
    float throttle,
    float valve,
    uint32_t timeout_ms
) {
    if (auto* d = dev_or_fail("actuator_execute")) {
        return d->actuator_execute(tx_id, throttle, valve, timeout_ms);
    }
    return false;
}

bool PlatformHAL::downlink_queue(const void* data, size_t len) {
    if (auto* d = dev_or_fail("downlink_queue")) return d->downlink_queue(data, len);
    return false;
}

void PlatformHAL::metric_emit(const char* name, float value) {
    if (auto* d = dev_or_null()) d->metric_emit(name, value);
}

void PlatformHAL::metric_emit(const char* name, float value, const char* tag_key, const char* tag_value) {
    if (auto* d = dev_or_null()) d->metric_emit(name, value, tag_key, tag_value);
}

// RNG helpers remain available for tests, but in HIL you typically don’t use them.
// We keep behavior consistent with the header contract.

std::mt19937_64 PlatformHAL::rng_{};
bool PlatformHAL::rng_seeded_ = false;

void PlatformHAL::seed_rng_for_stubs(uint32_t seed) {
    rng_.seed(static_cast<uint64_t>(seed));
    rng_seeded_ = true;
}

float PlatformHAL::random_float(float min, float max) {
    if (!rng_seeded_) seed_rng_for_stubs(1);
    if (max < min) std::swap(min, max);
    std::uniform_real_distribution<float> dist(min, max);
    return dist(rng_);
}

std::string PlatformHAL::generate_tx_id() {
    if (!rng_seeded_) seed_rng_for_stubs(1);

    static constexpr char HEX[] = "0123456789abcdef";
    std::uniform_int_distribution<int> dist(0, 15);

    std::string s;
    s.reserve(24);
    for (int i = 0; i < 24; ++i) s.push_back(HEX[dist(rng_)]);
    return s;
}
