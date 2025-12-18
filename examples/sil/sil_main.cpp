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

int main() {
#if (RAPS_ENABLE_SIL != 1)
    std::cerr << "RAPS_ENABLE_SIL is not enabled.\n";
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
        meta.build_type       = "SIL";
        meta.notes            = "SIL deterministic timing harness";

        raps::telemetry::write_telemetry_metadata(run_dir, meta);
    }

    // ------------------------------------------------------------
    // Telemetry: SIL lifecycle start
    // ------------------------------------------------------------
    {
        raps::telemetry::TelemetryEvent ev;
        ev.type      = raps::telemetry::EventType::ModeTransition;
        ev.subsystem = raps::telemetry::Subsystem::Core;
        ev.severity  = raps::telemetry::Severity::Info;
        ev.code      = 100; // SIL_START
        g_telemetry.emit(ev);
    }

    // ------------------------------------------------------------
    // SIL timing harness
    // - Deterministic
    // - No device injection
    // - PlatformHAL only
    // ------------------------------------------------------------
    const int hz = 50;
    const int period_ms = (hz > 0) ? (1000 / hz) : 20;

    std::cout << "=== SIL bring-up ===\n";
    std::cout << "now_ms: " << PlatformHAL::now_ms() << "\n";
    std::cout << "Running SIL timing harness at ~"
              << hz << " Hz for 2 seconds...\n";

    const uint32_t start = PlatformHAL::now_ms();
    uint32_t last_drain_ms = start;

    while (PlatformHAL::now_ms() - start < 2000u) {
        const uint32_t t0 = PlatformHAL::now_ms();

        // --------------------------------------------------------
        // Place real controller cycle here (future):
        // supervisor.run_cycle(...);
        // --------------------------------------------------------

        const uint32_t elapsed = PlatformHAL::now_ms() - t0;

        // --------------------------------------------------------
        // Deadline monitoring (observational only)
        // --------------------------------------------------------
        if (elapsed > static_cast<uint32_t>(period_ms)) {
            raps::telemetry::TelemetryEvent ev;
            ev.type      = raps::telemetry::EventType::ThresholdCross;
            ev.subsystem = raps::telemetry::Subsystem::Core;
            ev.severity  = raps::telemetry::Severity::Warn;
            ev.code      = 101;         // SIL_DEADLINE_MISS
            ev.v0        = elapsed;     // actual ms
            ev.v1        = period_ms;   // target ms
            g_telemetry.emit(ev);
        }

        // --------------------------------------------------------
        // Telemetry drain (safe cadence, non-blocking)
        // --------------------------------------------------------
        const uint32_t now = PlatformHAL::now_ms();
        if (now - last_drain_ms >= 250u) {
            g_telemetry.drain_to(telemetry_sink);
            telemetry_sink.flush();
            last_drain_ms = now;
        }

        int sleep_ms = period_ms - static_cast<int>(elapsed);
        if (sleep_ms < 0) sleep_ms = 0;
        std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));
    }

    // ------------------------------------------------------------
    // Telemetry: SIL lifecycle end
    // ------------------------------------------------------------
    {
        raps::telemetry::TelemetryEvent ev;
        ev.type      = raps::telemetry::EventType::ModeTransition;
        ev.subsystem = raps::telemetry::Subsystem::Core;
        ev.severity  = raps::telemetry::Severity::Info;
        ev.code      = 102; // SIL_STOP
        g_telemetry.emit(ev);
    }

    // Final drain before exit
    g_telemetry.drain_to(telemetry_sink);
    telemetry_sink.flush();

    std::cout << "SIL harness complete.\n";
    return 0;
}
