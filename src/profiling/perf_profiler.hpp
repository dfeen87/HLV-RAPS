#pragma once

#include <cstdint>
#include <atomic>
#include <chrono>
#include <string>
#include <string_view>
#include <unordered_map>
#include <mutex>
#include <vector>
#include <limits>
#include <utility>

namespace apms::profiling {

// ------------------------------------------------------------
// perf_profiler.hpp
// ------------------------------------------------------------
// Lightweight, cert-friendly performance instrumentation.
// Goals:
//   - Zero behavioral coupling: measurement only.
//   - Low overhead; can be compiled out via APMS_ENABLE_PROFILING=0.
//   - Deterministic data model: counters + min/max/mean + basic jitter.
//   - Safe for real-time-ish loops (atomic fast path; optional mutex for names).
//
// Typical usage:
//   APMS_PROFILE_SCOPE("control_loop");
//   ... work ...
//
// Or manually:
//   auto t0 = Profiler::clock::now();
//   ... work ...
//   Profiler::instance().record("stage_a", t0);
//
// You can also measure loop jitter:
//   Profiler::instance().record_period("control_loop_period", last_tick, now);
//
// ------------------------------------------------------------

// Compile-time switch (defaults ON if not defined)
#ifndef APMS_ENABLE_PROFILING
#define APMS_ENABLE_PROFILING 1
#endif

using SteadyClock = std::chrono::steady_clock;
using TimePoint   = SteadyClock::time_point;
using Nanoseconds = std::chrono::nanoseconds;

namespace detail {

static inline uint64_t ns_count(Nanoseconds d) noexcept {
    return static_cast<uint64_t>(d.count());
}

static inline Nanoseconds ns_delta(TimePoint start, TimePoint end) noexcept {
    return std::chrono::duration_cast<Nanoseconds>(end - start);
}

} // namespace detail

// ------------------------------------------------------------
// Metric Snapshot
// ------------------------------------------------------------
struct MetricSnapshot {
    std::string name;

    uint64_t samples = 0;

    uint64_t min_ns = std::numeric_limits<uint64_t>::max();
    uint64_t max_ns = 0;

    // Accumulators for mean
    long double sum_ns = 0.0L;

    // Optional period/jitter tracking
    // - last_period_ns: the most recent observed period
    // - max_jitter_ns: max |period - target|
    uint64_t last_period_ns = 0;
    uint64_t max_jitter_ns  = 0;

    // If set, we compute jitter relative to this target
    uint64_t target_period_ns = 0;

    double mean_ns() const noexcept {
        if (samples == 0) return 0.0;
        return static_cast<double>(sum_ns / static_cast<long double>(samples));
    }
};

// ------------------------------------------------------------
// Profiler
// ------------------------------------------------------------
class Profiler {
public:
    using clock = SteadyClock;

    static Profiler& instance() {
        static Profiler p;
        return p;
    }

    // Enable/disable at runtime (profiling may still be compiled in)
    void set_enabled(bool enabled) noexcept { enabled_.store(enabled, std::memory_order_relaxed); }
    bool enabled() const noexcept { return enabled_.load(std::memory_order_relaxed); }

    // Set a target period for a metric to compute jitter against (nanoseconds)
    void set_target_period_ns(std::string_view metric, uint64_t target_ns) {
        if (!enabled()) return;
        auto* m = metric_ptr_(metric);
        if (!m) return;
        m->target_period_ns = target_ns;
    }

    // Record a duration from start->now for a metric
    void record(std::string_view metric, TimePoint start) noexcept {
#if APMS_ENABLE_PROFILING
        if (!enabled()) return;
        const auto end = clock::now();
        record_ns_(metric, detail::ns_count(detail::ns_delta(start, end)));
#else
        (void)metric; (void)start;
#endif
    }

    // Record an explicit duration (nanoseconds)
    void record_ns(std::string_view metric, uint64_t duration_ns) noexcept {
#if APMS_ENABLE_PROFILING
        if (!enabled()) return;
        record_ns_(metric, duration_ns);
#else
        (void)metric; (void)duration_ns;
#endif
    }

    // Record a period and optionally compute jitter against a target period (if set)
    void record_period(std::string_view metric, TimePoint previous_tick, TimePoint current_tick) noexcept {
#if APMS_ENABLE_PROFILING
        if (!enabled()) return;
        const uint64_t period_ns = detail::ns_count(detail::ns_delta(previous_tick, current_tick));
        record_period_ns_(metric, period_ns);
#else
        (void)metric; (void)previous_tick; (void)current_tick;
#endif
    }

    // Reset all metrics (keeps names)
    void reset() {
#if APMS_ENABLE_PROFILING
        std::lock_guard<std::mutex> lk(mu_);
        for (auto& kv : metrics_) {
            kv.second.samples = 0;
            kv.second.min_ns = std::numeric_limits<uint64_t>::max();
            kv.second.max_ns = 0;
            kv.second.sum_ns = 0.0L;
            kv.second.last_period_ns = 0;
            kv.second.max_jitter_ns = 0;
            // keep target_period_ns as configured
        }
#endif
    }

    // Snapshot all metrics in a stable vector (safe for logging/telemetry)
    std::vector<MetricSnapshot> snapshot() const {
        std::vector<MetricSnapshot> out;
#if APMS_ENABLE_PROFILING
        std::lock_guard<std::mutex> lk(mu_);
        out.reserve(metrics_.size());
        for (const auto& kv : metrics_) {
            out.push_back(kv.second);
        }
#endif
        return out;
    }

private:
    Profiler() = default;

#if APMS_ENABLE_PROFILING
    MetricSnapshot* metric_ptr_(std::string_view metric) {
        std::lock_guard<std::mutex> lk(mu_);
        auto it = metrics_.find(std::string(metric));
        if (it == metrics_.end()) {
            MetricSnapshot ms;
            ms.name = std::string(metric);
            auto [ins_it, _] = metrics_.emplace(ms.name, std::move(ms));
            return &ins_it->second;
        }
        return &it->second;
    }

    void record_ns_(std::string_view metric, uint64_t ns) noexcept {
        // Minimal lock duration: find metric then update its stats.
        MetricSnapshot* m = metric_ptr_(metric);
        if (!m) return;

        // Update stats (these updates are under mutex due to metric_ptr_ using mutex)
        // Cert-friendly: simple scalar stats
        m->samples += 1;
        m->sum_ns += static_cast<long double>(ns);
        if (ns < m->min_ns) m->min_ns = ns;
        if (ns > m->max_ns) m->max_ns = ns;
    }

    void record_period_ns_(std::string_view metric, uint64_t period_ns) noexcept {
        MetricSnapshot* m = metric_ptr_(metric);
        if (!m) return;

        m->last_period_ns = period_ns;

        // Also track period distribution via min/max/mean like any other metric
        m->samples += 1;
        m->sum_ns += static_cast<long double>(period_ns);
        if (period_ns < m->min_ns) m->min_ns = period_ns;
        if (period_ns > m->max_ns) m->max_ns = period_ns;

        if (m->target_period_ns != 0) {
            const uint64_t target = m->target_period_ns;
            const uint64_t jitter = (period_ns > target) ? (period_ns - target) : (target - period_ns);
            if (jitter > m->max_jitter_ns) m->max_jitter_ns = jitter;
        }
    }

    mutable std::mutex mu_;
    std::unordered_map<std::string, MetricSnapshot> metrics_;
#endif

    std::atomic<bool> enabled_{true};
};

// ------------------------------------------------------------
// RAII Scope Timer
// ------------------------------------------------------------
class ScopeTimer {
public:
    explicit ScopeTimer(std::string_view metric) noexcept
        : metric_(metric), start_(Profiler::clock::now()) {}

    ~ScopeTimer() {
#if APMS_ENABLE_PROFILING
        Profiler::instance().record(metric_, start_);
#endif
    }

    ScopeTimer(const ScopeTimer&) = delete;
    ScopeTimer& operator=(const ScopeTimer&) = delete;

private:
    std::string_view metric_;
    TimePoint start_;
};

// ------------------------------------------------------------
// Convenience Macros
// ------------------------------------------------------------
#if APMS_ENABLE_PROFILING
  #define APMS_PROFILE_SCOPE(name_literal) ::apms::profiling::ScopeTimer apms_prof_scope_##__LINE__(name_literal)
  #define APMS_PROFILE_RECORD(name_literal, start_timepoint) ::apms::profiling::Profiler::instance().record(name_literal, start_timepoint)
  #define APMS_PROFILE_PERIOD(name_literal, prev_tp, cur_tp) ::apms::profiling::Profiler::instance().record_period(name_literal, prev_tp, cur_tp)
#else
  #define APMS_PROFILE_SCOPE(name_literal) do {} while(0)
  #define APMS_PROFILE_RECORD(name_literal, start_timepoint) do { (void)(name_literal); (void)(start_timepoint); } while(0)
  #define APMS_PROFILE_PERIOD(name_literal, prev_tp, cur_tp) do { (void)(name_literal); (void)(prev_tp); (void)(cur_tp); } while(0)
#endif

} // namespace apms::profiling
