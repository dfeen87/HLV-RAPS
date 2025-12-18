#pragma once
/*
 * Production telemetry logger.
 * - Bounded queue
 * - No allocations in emit() hot path
 * - Best-effort ordering via seq counter
 *
 * License: MIT (see LICENSE)
 */

#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstring>

#include "telemetry_event.hpp"
#include "telemetry_ring_buffer.hpp"

namespace raps::telemetry {

struct TelemetryConfig final {
  // Whether to include wall time stamps (slightly more overhead).
  bool enable_wall_time = false;

  // Severity floor; events below this are ignored.
  Severity min_severity = Severity::Info;

  // If true, Message events (type=Message) copy msg; if false msg is ignored.
  bool enable_messages = true;
};

// CapacityPow2 example: 4096 events. Adjust to taste.
// Keep power-of-two.
template <size_t CapacityPow2 = 4096>
class TelemetryLogger final {
public:
  using Ring = TelemetryRingBuffer<TelemetryEvent, CapacityPow2>;

  explicit TelemetryLogger(TelemetryConfig cfg = {}) noexcept
    : _cfg(cfg),
      _seq(0),
      _t0(std::chrono::steady_clock::now()) {}

  // Fast emit. Never throws. Bounded. Drops when full.
  void emit(TelemetryEvent ev) noexcept {
    if (ev.severity < _cfg.min_severity) return;

    ev.seq = _seq.fetch_add(1, std::memory_order_relaxed);

    const auto now = std::chrono::steady_clock::now();
    ev.t_mono_ns = (uint64_t)std::chrono::duration_cast<std::chrono::nanoseconds>(now - _t0).count();

    if (_cfg.enable_wall_time) {
      // system_clock can jump; used only for correlation.
      const auto wall = std::chrono::system_clock::now().time_since_epoch();
      ev.t_wall_ns = (uint64_t)std::chrono::duration_cast<std::chrono::nanoseconds>(wall).count();
    } else {
      ev.t_wall_ns = 0;
    }

    if (!_cfg.enable_messages) {
      ev.msg[0] = '\0';
    } else {
      // Ensure null termination.
      ev.msg[TelemetryEvent::MSG_CAP - 1] = '\0';
    }

    (void)_ring.try_push(ev);
  }

  // Convenience helper for message events.
  void message(Subsystem ss, Severity sev, uint32_t code, const char* text) noexcept {
    TelemetryEvent ev;
    ev.type = EventType::Message;
    ev.subsystem = ss;
    ev.severity = sev;
    ev.code = code;

    if (text && _cfg.enable_messages) {
      std::strncpy(ev.msg, text, TelemetryEvent::MSG_CAP - 1);
      ev.msg[TelemetryEvent::MSG_CAP - 1] = '\0';
    } else {
      ev.msg[0] = '\0';
    }

    emit(ev);
  }

  // Drain events to a sink. Typically called from a dedicated consumer thread
  // or at safe points in your main loop.
  template <typename SinkT>
  uint64_t drain_to(SinkT& sink, uint64_t max_events = 0) noexcept {
    uint64_t n = 0;
    TelemetryEvent ev;
    while (_ring.try_pop(ev)) {
      sink.on_event(ev);
      ++n;
      if (max_events != 0 && n >= max_events) break;
    }
    sink.on_dropped(_ring.dropped());
    return n;
  }

  uint64_t dropped_total() const noexcept { return _ring.dropped(); }
  uint64_t queued_approx()  const noexcept { return _ring.size_approx(); }

private:
  TelemetryConfig _cfg;
  std::atomic<uint64_t> _seq;
  std::chrono::steady_clock::time_point _t0;
  Ring _ring;
};

} // namespace raps::telemetry
