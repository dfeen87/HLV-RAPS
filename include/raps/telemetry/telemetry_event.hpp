#pragma once
/*
 * RAPS Telemetry v2.3
 * Deterministic, bounded, read-only observability events.
 *
 * License: MIT (see LICENSE)
 */

#include <cstdint>

namespace raps::telemetry {

enum class Severity : uint8_t {
  Debug = 0,
  Info  = 1,
  Warn  = 2,
  Error = 3,
  Fatal = 4
};

// Keep stable: these become part of the observability contract.
enum class EventType : uint16_t {
  Heartbeat        = 1,
  LoopTiming       = 2,
  ModeTransition   = 3,
  SafetyGate       = 4,
  ThresholdCross   = 5,
  InputMetrics     = 6,
  Counter          = 7,
  Message          = 8
};

// Stable subsystem identifiers (extend as needed).
enum class Subsystem : uint16_t {
  Core        = 1,
  Sensors     = 2,
  DSP         = 3,
  Control     = 4,
  Safety      = 5,
  IO          = 6,
  Storage     = 7,
  Diagnostics = 8
};

// A compact POD event. No heap. No std::string.
struct TelemetryEvent final {
  // Monotonic timestamp in nanoseconds since an arbitrary start.
  // Use steady_clock to avoid wall-clock jumps.
  uint64_t t_mono_ns = 0;

  // Optional wall clock timestamp in unix epoch nanoseconds.
  // Can be 0 if not available / not desired.
  uint64_t t_wall_ns = 0;

  // Sequence number assigned by logger (monotonic best-effort).
  uint64_t seq = 0;

  EventType  type = EventType::Message;
  Subsystem  subsystem = Subsystem::Core;
  Severity   severity = Severity::Info;

  // Stable numeric code (e.g., gate id, threshold id, error id).
  uint32_t code = 0;

  // Up to 3 numeric values (interpretation depends on type).
  // Examples:
  // - LoopTiming: v0=dt_us, v1=jitter_us, v2=load_x1000
  // - InputMetrics: v0=rms_x1000, v1=peak_x1000, v2=sat_flag
  int64_t v0 = 0;
  int64_t v1 = 0;
  int64_t v2 = 0;

  // Optional short message. Fixed-size to avoid allocations.
  // Null-terminated if shorter.
  static constexpr uint32_t MSG_CAP = 64;
  char msg[MSG_CAP] = {0};
};

} // namespace raps::telemetry
