#include "raps/hil/hil_config.hpp"
#include "raps/hil/hil_device_interface.hpp"
#include "platform/platform_hal.hpp"

// --- Telemetry (v2.3) ---
#include "raps/telemetry/telemetry_logger.hpp"
#include "raps/telemetry/jsonl_sink.hpp"
#include "raps/telemetry/telemetry_run_directory.hpp"
#include "raps/telemetry/telemetry_metadata.hpp"

#include <chrono>
#include <cstring>
#include <iostream>
#include <thread>

// ------------------------------------------------------------
// Telemetry logger (bounded, non-blocking, best-effort)
// ------------------------------------------------------------
static raps::telemetry::TelemetryLogger<4096> g_telemetry(
    raps::telemetry::TelemetryConfig{
        .enable_wall_time = false,   // monotonic only
        .min_severity     = raps::telemetry::Severity::Info,
        .enable_messages  = true
    }
);

// ------------------------------------------------------------
// Minimal loopback “device” for HIL bring-up
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
            h.data[i] =
                static_cast<uint8_t>((sum >> ((i % 8) * 8)) & 0xFF)
                ^ static_cast<uint8_t>(i * 31);
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

    // ------------------------------------------------------------
    // Telemetry initialization (best-effort, non-fatal)
    // ------------------------------------------------------------
    const std::string run_dir = raps::telemetry::create_run_directory();
    raps::telemetry::JsonlSink telemetry_sink;

    if (!run_dir.empty()) {
        telemetry_sink.open((run_dir + "/telemetry.jsonl").c_str());

        raps::telemetry::TelemetryMetadata meta;
        meta.raps_version     = "2.3.0";
        meta.telemetry_schema = "1.0";
        meta.build_type       = "HIL";
        meta.notes            = "HIL loopback bring-up";

        raps::telemetry::write_telemetry_metadata(run_dir, meta);
    }

    // Inject the device before anything calls PlatformHAL.
    LoopbackHilDevice dev;
    hil_set_device(&dev);

    // Telemetry: HIL start
    {
        raps::telemetry::TelemetryEvent ev;
        ev.type      = raps::telemetry::EventType::ModeTransition;
        ev.subsystem = raps::telemetry::Subsystem::HIL;
        ev.severity  = raps::telemetry::Severity::Info;
        ev.code      = 1; // HIL_START
        g_telemetry.emit(ev);
    }

    // Basic smoke test
    const char* msg = "HIL_SMOKE_TEST";
    Hash256 h = PlatformHAL::sha256(msg, std::strlen(msg));
    uint8_t sig[64]{};
    bool ok = PlatformHAL::ed25519_sign(h, sig);

    std::cout << "=== HIL bring-up ===\n";
    std::cout << "now_ms: " << PlatformHAL::now_ms() << "\n";
    std::cout << "ed25519_sign: " << (ok ? "OK" : "FAIL") << "\n";
    std::cout << "actuator_execute: "
              << (PlatformHAL::actuator_execute(
                     "tx_demo_001", 98.0f, -0.02f,
                     RAPS_HIL_ACTUATOR_TIMEOUT_MS) ? "OK" : "FAIL")
              << "\n";

    // Cycle timing harness
    const int hz = RAPS_HIL_CYCLE_HZ;
    const int period_ms = (hz > 0) ? (1000 / hz) : 20;

    std::cout << "Running HIL timing harness at ~" << hz
              << " Hz for 2 seconds...\n";

    uint32_t start = PlatformHAL::now_ms();

    while (PlatformHAL::now_ms() - start < 2000u) {
        uint32_t t0 = PlatformHAL::now_ms();

        // supervisor.run_cycle(...);

        uint32_t elapsed = PlatformHAL::now_ms() - t0;
        if (elapsed > static_cast<uint32_t>(period_ms)) {
            PlatformHAL::metric_emit("hil.deadline_miss", 1.0f);

            raps::telemetry::TelemetryEvent ev;
            ev.type      = raps::telemetry::EventType::ThresholdCross;
            ev.subsystem = raps::telemetry::Subsystem::HIL;
            ev.severity  = raps::telemetry::Severity::Warn;
            ev.code      = 1;           // DEADLINE_MISS
            ev.v0        = elapsed;     // actual ms
            ev.v1        = period_ms;   // target ms
            g_telemetry.emit(ev);
        }

        // Drain telemetry at safe cadence (~4 Hz)
        if ((PlatformHAL::now_ms() - start) % 250u == 0u) {
            g_telemetry.drain_to(telemetry_sink);
            telemetry_sink.flush();
        }

        int sleep_ms = period_ms - static_cast<int>(elapsed);
        if (sleep_ms < 0) sleep_ms = 0;
        std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));
    }

    // Final drain before exit
    g_telemetry.drain_to(telemetry_sink);
    telemetry_sink.flush();

    std::cout << "HIL harness complete.\n";
    return 0;
}
