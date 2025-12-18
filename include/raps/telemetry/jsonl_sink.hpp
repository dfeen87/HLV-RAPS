#pragma once
/*
 * JSON Lines sink: one event per line.
 * - Append-only
 * - Easy to parse
 * - Stream-friendly
 *
 * License: MIT (see LICENSE)
 */

#include <cstdio>
#include <cstdint>
#include <cstring>

#include "telemetry_sink.hpp"

namespace raps::telemetry {

inline const char* to_string(Severity s) noexcept {
  switch (s) {
    case Severity::Debug: return "debug";
    case Severity::Info:  return "info";
    case Severity::Warn:  return "warn";
    case Severity::Error: return "error";
    case Severity::Fatal: return "fatal";
    default:              return "info";
  }
}

inline const char* to_string(EventType t) noexcept {
  switch (t) {
    case EventType::Heartbeat:      return "heartbeat";
    case EventType::LoopTiming:     return "loop_timing";
    case EventType::ModeTransition: return "mode_transition";
    case EventType::SafetyGate:     return "safety_gate";
    case EventType::ThresholdCross: return "threshold_cross";
    case EventType::InputMetrics:   return "input_metrics";
    case EventType::Counter:        return "counter";
    case EventType::Message:        return "message";
    default:                        return "message";
  }
}

inline const char* to_string(Subsystem s) noexcept {
  switch (s) {
    case Subsystem::Core:        return "core";
    case Subsystem::Sensors:     return "sensors";
    case Subsystem::DSP:         return "dsp";
    case Subsystem::Control:     return "control";
    case Subsystem::Safety:      return "safety";
    case Subsystem::IO:          return "io";
    case Subsystem::Storage:     return "storage";
    case Subsystem::Diagnostics: return "diagnostics";
    default:                     return "core";
  }
}

// Very small JSON escaping for msg field (only quotes, backslash, control chars).
inline void json_escape(FILE* f, const char* s) noexcept {
  if (!s) { std::fputs("", f); return; }
  for (const unsigned char* p = (const unsigned char*)s; *p; ++p) {
    switch (*p) {
      case '\\': std::fputs("\\\\", f); break;
      case '\"': std::fputs("\\\"", f); break;
      case '\n': std::fputs("\\n", f); break;
      case '\r': std::fputs("\\r", f); break;
      case '\t': std::fputs("\\t", f); break;
      default:
        if (*p < 0x20) {
          // control char -> \u00XX
          std::fprintf(f, "\\u%04x", (unsigned int)(*p));
        } else {
          std::fputc(*p, f);
        }
    }
  }
}

class JsonlSink final : public ITelemetrySink {
public:
  explicit JsonlSink(const char* path) noexcept : _f(nullptr) {
    if (path && path[0]) {
      _f = std::fopen(path, "ab"); // append, binary safe
    }
  }

  ~JsonlSink() override {
    if (_f) std::fclose(_f);
    _f = nullptr;
  }

  bool ok() const noexcept { return _f != nullptr; }

  void on_event(const TelemetryEvent& ev) noexcept override {
    if (!_f) return;

    // One object per line
    std::fputs("{\"seq\":", _f); std::fprintf(_f, "%llu", (unsigned long long)ev.seq);
    std::fputs(",\"t_mono_ns\":", _f); std::fprintf(_f, "%llu", (unsigned long long)ev.t_mono_ns);
    std::fputs(",\"t_wall_ns\":", _f); std::fprintf(_f, "%llu", (unsigned long long)ev.t_wall_ns);

    std::fputs(",\"type\":\"", _f); std::fputs(to_string(ev.type), _f); std::fputs("\"", _f);
    std::fputs(",\"subsystem\":\"", _f); std::fputs(to_string(ev.subsystem), _f); std::fputs("\"", _f);
    std::fputs(",\"severity\":\"", _f); std::fputs(to_string(ev.severity), _f); std::fputs("\"", _f);

    std::fputs(",\"code\":", _f); std::fprintf(_f, "%u", ev.code);

    std::fputs(",\"v0\":", _f); std::fprintf(_f, "%lld", (long long)ev.v0);
    std::fputs(",\"v1\":", _f); std::fprintf(_f, "%lld", (long long)ev.v1);
    std::fputs(",\"v2\":", _f); std::fprintf(_f, "%lld", (long long)ev.v2);

    std::fputs(",\"msg\":\"", _f); json_escape(_f, ev.msg); std::fputs("\"}\n", _f);
  }

  void on_dropped(uint64_t dropped_total) noexcept override {
    if (!_f) return;
    // periodic summary marker
    std::fputs("{\"type\":\"telemetry_summary\",\"dropped_total\":", _f);
    std::fprintf(_f, "%llu", (unsigned long long)dropped_total);
    std::fputs("}\n", _f);
  }

  void flush() noexcept {
    if (_f) std::fflush(_f);
  }

private:
  FILE* _f;
};

} // namespace raps::telemetry
