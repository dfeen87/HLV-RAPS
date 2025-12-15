#include "raps/hil/hil_config.hpp"
#include "raps/hil/hil_device_interface.hpp"
#include "platform/platform_hal.hpp"

#include <chrono>
#include <cstring>
#include <iostream>
#include <thread>

// ------------------------------------------------------------
// Minimal loopback “device” for HIL bring-up
// - Replace with TCP/Serial/CAN driver later
// - Purpose: validate the HAL delegation path end-to-end
// ------------------------------------------------------------

class LoopbackHilDevice final : public HilDeviceInterface {
public:
    uint32_t now_ms() override {
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()
        ).count();
        return static_cast<uint32_t>(ms);
    }

    Hash256 sha256(const void* data, size_t len) override {
        // Use the same placeholder approach as SIL (deterministic, non-crypto)
        Hash256 h{};
        std::memset(h.data, 0, sizeof(h.data));
        if (!data || len == 0) return h;

        uint64_t sum = 0;
        const uint8_t* b = static_cast<const uint8_t*>(data);
        for (size_t i = 0; i < len; ++i) {
            sum = (sum * 1315423911u) ^ b[i] ^ (sum >> 16);
        }

        std::memcpy(h.data, &sum, std::min(sizeof(sum), sizeof(h.data)));
        h.data[8]  = static_cast<uint8_t>(len & 0xFF);
        h.data[9]  = static_cast<uint8_t>((len >> 8) & 0xFF);
        for (size_t i = 10; i < 32; ++i) {
            h.data[i] = static_cast<uint8_t>((sum >> ((i % 8) * 8)) & 0xFF) ^ static_cast<uint8_t>(i * 31);
        }
        return h;
    }

    bool ed25519_sign(const Hash256&, uint8_t signature[64]) override {
        if (!signature) return false;
        std::memset(signature, 0xCD, 64);
        return true;
    }

    bool flash_write(uint32_t, const void*, size_t) override { return true; }
    bool flash_read(uint32_t, void* data, size_t len) override {
        if (!data) return false;
        std::memset(data, 0, len);
        return true;
    }

    bool actuator_execute(const char* tx_id, float throttle, float valve, uint32_t timeout_ms) override {
        (void)timeout_ms;

        if (!tx_id || tx_id[0] == '\0') return false;

#if RAPS_HIL_VERBOSE_IO
        std::cout << "[HIL] actuator_execute tx_id=" << tx_id
                  << " throttle=" << throttle
                  << " valve=" << valve
                  << " timeout_ms=" << timeout_ms << "\n";
#else
        (void)throttle;
        (void)valve;
#endif
        return true;
    }

    bool downlink_queue(const void*, size_t) override { return true; }

    void metric_emit(const char* name, float value) override {
#if RAPS_HIL_VERBOSE_IO
        std::cout << "[METRIC] " << name << "=" << value << "\n";
#else
        (void)name; (void)value;
#endif
    }

    void metric_emit(const char* name, float value, const char* k, const char* v) override {
#if RAPS_HIL_VERBOSE_IO
        std::cout << "[METRIC] " << name << "=" << value << " " << k << "=" << v << "\n";
#else
        (void)name; (void)value; (void)k; (void)v;
#endif
    }
};

int main() {
#if (RAPS_ENABLE_HIL != 1)
    std::cerr << "RAPS_ENABLE_HIL is not enabled.\n";
    return 2;
#endif

    // Inject the device before anything calls PlatformHAL.
    LoopbackHilDevice dev;
    hil_set_device(&dev);

    // Basic smoke test: verify PlatformHAL routes to the HIL device.
    const char* msg = "HIL_SMOKE_TEST";
    Hash256 h = PlatformHAL::sha256(msg, std::strlen(msg));
    uint8_t sig[64]{};
    bool ok = PlatformHAL::ed25519_sign(h, sig);

    std::cout << "=== HIL bring-up ===\n";
    std::cout << "now_ms: " << PlatformHAL::now_ms() << "\n";
    std::cout << "ed25519_sign: " << (ok ? "OK" : "FAIL") << "\n";
    std::cout << "actuator_execute: "
              << (PlatformHAL::actuator_execute("tx_demo_001", 98.0f, -0.02f, RAPS_HIL_ACTUATOR_TIMEOUT_MS) ? "OK" : "FAIL")
              << "\n";

    // Cycle timing harness (no RAPS loop here yet; this is the HIL scaffold)
    const int hz = RAPS_HIL_CYCLE_HZ;
    const int period_ms = (hz > 0) ? (1000 / hz) : 20;

    std::cout << "Running HIL timing harness at ~" << hz << " Hz for 2 seconds...\n";
    uint32_t start = PlatformHAL::now_ms();
    while (PlatformHAL::now_ms() - start < 2000u) {
        uint32_t t0 = PlatformHAL::now_ms();

        // Place your real controller cycle call here later:
        // supervisor.run_cycle(...);

        uint32_t elapsed = PlatformHAL::now_ms() - t0;
        if (elapsed > static_cast<uint32_t>(period_ms)) {
            PlatformHAL::metric_emit("hil.deadline_miss", 1.0f);
        }

        int sleep_ms = period_ms - static_cast<int>(elapsed);
        if (sleep_ms < 0) sleep_ms = 0;
        std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));
    }

    std::cout << "HIL harness complete.\n";
    return 0;
}
