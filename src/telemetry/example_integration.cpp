#include "raps/telemetry/telemetry_logger.hpp"
#include "raps/telemetry/jsonl_sink.hpp"

using raps::telemetry::TelemetryLogger;
using raps::telemetry::TelemetryConfig;
using raps::telemetry::TelemetryEvent;
using raps::telemetry::EventType;
using raps::telemetry::Subsystem;
using raps::telemetry::Severity;

static TelemetryLogger<4096> g_telemetry(
  TelemetryConfig{
    .enable_wall_time = false,   // optional
    .min_severity = Severity::Info,
    .enable_messages = true
  }
);

int main(int argc, char** argv) {
  raps::telemetry::JsonlSink sink("raps.telemetry.jsonl");
  if (!sink.ok()) {
    // If sink fails, telemetry still works; it will just drop drains.
  }

  // Example: heartbeat
  {
    TelemetryEvent ev;
    ev.type = EventType::Heartbeat;
    ev.subsystem = Subsystem::Core;
    ev.severity = Severity::Info;
    ev.code = 1;     // heartbeat id
    ev.v0 = 1;       // alive
    g_telemetry.emit(ev);
  }

  // Main loop example (pseudo)
  for (int i = 0; i < 1000; ++i) {
    // ... do work ...

    // Example: loop timing
    {
      TelemetryEvent ev;
      ev.type = EventType::LoopTiming;
      ev.subsystem = Subsystem::Core;
      ev.severity = Severity::Info;
      ev.code = 0;     // loop id
      ev.v0 = 1000;    // dt_us (example)
      ev.v1 = 25;      // jitter_us
      ev.v2 = 700;     // load_x1000 (0.700)
      g_telemetry.emit(ev);
    }

    // Drain at safe cadence (every N iterations) OR in separate thread.
    if ((i % 50) == 0) {
      g_telemetry.drain_to(sink, /*max_events=*/0);
      sink.flush();
    }
  }

  // Final drain before exit
  g_telemetry.drain_to(sink);
  sink.flush();
  return 0;
}
