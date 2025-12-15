#pragma once

// ============================================================================
// SIL Coverage Gates
// ----------------------------------------------------------------------------
// Purpose:
//  - Enforce that critical safety paths are exercised in SIL/CI
//  - Provide simple, header-only counters + hard fail checks
//  - Keep the mechanism lightweight and dependency-free
//
// Usage pattern:
//  1) In SIL code paths, increment gates:
//       RAPS_SIL_COVER("rollback.executed");
//       RAPS_SIL_COVER("supervisor.failover");
//  2) At end of a test (or demo main), call:
//       raps::sil::coverage::assert_minimum_coverage_or_abort();
//
// Build controls:
//   -DRAPS_ENABLE_SIL_COVERAGE_GATES=1  (default ON when SIL faults enabled)
//   -DRAPS_ENABLE_SIL_FAULTS=1         (optional, but usually paired)
//
// Notes:
//  - No overhead in flight builds unless explicitly enabled.
//  - Thread-safe via atomics.
// ============================================================================

#include <atomic>
#include <cstdint>
#include <cstdlib>
#include <cstring>

#ifndef RAPS_ENABLE_SIL_COVERAGE_GATES
  #if defined(RAPS_ENABLE_SIL_FAULTS) && (RAPS_ENABLE_SIL_FAULTS == 1)
    #define RAPS_ENABLE_SIL_COVERAGE_GATES 1
  #else
    #define RAPS_ENABLE_SIL_COVERAGE_GATES 0
  #endif
#endif

namespace raps {
namespace sil {
namespace coverage {

#if RAPS_ENABLE_SIL_COVERAGE_GATES

// ----------------------------------------------------------------------------
// Gate IDs (fixed set to keep things deterministic and header-only)
// ----------------------------------------------------------------------------
enum class GateId : uint8_t {
    // Core fault/recovery paths
    ROLLBACK_EXECUTED = 0,
    FALLBACK_TRIGGERED,
    EXECUTION_FAILURE,
    ACTUATOR_TIMEOUT_OR_FAIL,

    // Supervisor / redundancy
    SUPERVISOR_FAILOVER,
    SUPERVISOR_EXCEPTION_LOGGED,
    PREDICTION_MISMATCH_DETECTED,

    // ITL / Merkle anchoring
    ITL_COMMIT,
    ITL_FLUSH,
    ITL_MERKLE_ANCHOR,

    // Keep last
    COUNT
};

struct GateCounters {
    std::atomic<uint32_t> counts[static_cast<uint8_t>(GateId::COUNT)];

    constexpr GateCounters() : counts{} {}

    inline void hit(GateId id, uint32_t n = 1) {
        counts[static_cast<uint8_t>(id)].fetch_add(n, std::memory_order_relaxed);
    }

    inline uint32_t get(GateId id) const {
        return counts[static_cast<uint8_t>(id)].load(std::memory_order_relaxed);
    }

    inline void reset() {
        for (auto& c : counts) {
            c.store(0, std::memory_order_relaxed);
        }
    }
};

// Global singleton (header-only, function-local static)
inline GateCounters& gates() {
    static GateCounters g{};
    return g;
}

// ----------------------------------------------------------------------------
// Mapping from string keys to GateId
// Keep the strings short and stable for CI.
// ----------------------------------------------------------------------------
inline bool gate_id_from_key(const char* key, GateId& out) {
    if (!key) return false;

    // Core fault/recovery paths
    if (std::strcmp(key, "rollback.executed") == 0) { out = GateId::ROLLBACK_EXECUTED; return true; }
    if (std::strcmp(key, "fallback.triggered") == 0) { out = GateId::FALLBACK_TRIGGERED; return true; }
    if (std::strcmp(key, "execution.failure") == 0) { out = GateId::EXECUTION_FAILURE; return true; }
    if (std::strcmp(key, "actuator.timeout_or_fail") == 0) { out = GateId::ACTUATOR_TIMEOUT_OR_FAIL; return true; }

    // Supervisor / redundancy
    if (std::strcmp(key, "supervisor.failover") == 0) { out = GateId::SUPERVISOR_FAILOVER; return true; }
    if (std::strcmp(key, "supervisor.exception_logged") == 0) { out = GateId::SUPERVISOR_EXCEPTION_LOGGED; return true; }
    if (std::strcmp(key, "supervisor.prediction_mismatch") == 0) { out = GateId::PREDICTION_MISMATCH_DETECTED; return true; }

    // ITL / Merkle anchoring
    if (std::strcmp(key, "itl.commit") == 0) { out = GateId::ITL_COMMIT; return true; }
    if (std::strcmp(key, "itl.flush") == 0) { out = GateId::ITL_FLUSH; return true; }
    if (std::strcmp(key, "itl.merkle_anchor") == 0) { out = GateId::ITL_MERKLE_ANCHOR; return true; }

    return false;
}

// ----------------------------------------------------------------------------
// Public helpers
// ----------------------------------------------------------------------------
inline void cover(const char* key, uint32_t n = 1) {
    GateId id{};
    if (gate_id_from_key(key, id)) {
        gates().hit(id, n);
    }
    // Unknown keys are ignored intentionally (keeps instrumentation robust).
}

// ----------------------------------------------------------------------------
// Coverage policy thresholds (tune as desired for CI)
// ----------------------------------------------------------------------------
struct CoveragePolicy {
    // Minimum hits expected at end of SIL run:
    uint32_t min_itl_commit = 1;
    uint32_t min_itl_flush = 1;
    uint32_t min_merkle_anchor = 0; // allow 0 if batches don't fill, unless forced

    // Safety/recovery gates:
    uint32_t min_fallback_triggered = 1;
    uint32_t min_execution_failure = 1;
    uint32_t min_rollback_executed = 1;

    // Supervisor gates (optional depending on test):
    uint32_t min_supervisor_failover = 0;
    uint32_t min_prediction_mismatch = 0;
};

inline CoveragePolicy& policy() {
    static CoveragePolicy p{};
    return p;
}

// ----------------------------------------------------------------------------
// Assertion behavior
// - Abort (CI-friendly) if minimums not met.
// ----------------------------------------------------------------------------
inline void assert_minimum_coverage_or_abort() {
    const auto& g = gates();
    const auto& p = policy();

    // ITL
    if (g.get(GateId::ITL_COMMIT) < p.min_itl_commit) std::abort();
    if (g.get(GateId::ITL_FLUSH)  < p.min_itl_flush)  std::abort();
    if (g.get(GateId::ITL_MERKLE_ANCHOR) < p.min_merkle_anchor) std::abort();

    // Fault/recovery
    if (g.get(GateId::FALLBACK_TRIGGERED) < p.min_fallback_triggered) std::abort();
    if (g.get(GateId::EXECUTION_FAILURE)  < p.min_execution_failure)  std::abort();
    if (g.get(GateId::ROLLBACK_EXECUTED)  < p.min_rollback_executed)  std::abort();

    // Supervisor (optional)
    if (g.get(GateId::SUPERVISOR_FAILOVER) < p.min_supervisor_failover) std::abort();
    if (g.get(GateId::PREDICTION_MISMATCH_DETECTED) < p.min_prediction_mismatch) std::abort();
}

#else // RAPS_ENABLE_SIL_COVERAGE_GATES == 0

// Compiled out in non-SIL builds
inline void cover(const char*, uint32_t = 1) {}
struct CoveragePolicy {};
inline CoveragePolicy& policy() { static CoveragePolicy p{}; return p; }
inline void assert_minimum_coverage_or_abort() {}

#endif // RAPS_ENABLE_SIL_COVERAGE_GATES

} // namespace coverage
} // namespace sil
} // namespace raps

// ----------------------------------------------------------------------------
// Convenience macro for instrumentation sites
// ----------------------------------------------------------------------------
#if RAPS_ENABLE_SIL_COVERAGE_GATES
  #define RAPS_SIL_COVER(KEY) ::raps::sil::coverage::cover((KEY), 1u)
  #define RAPS_SIL_COVER_N(KEY, N) ::raps::sil::coverage::cover((KEY), static_cast<uint32_t>(N))
#else
  #define RAPS_SIL_COVER(KEY) do {} while (0)
  #define RAPS_SIL_COVER_N(KEY, N) do { (void)(KEY); (void)(N); } while (0)
#endif
