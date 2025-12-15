// ============================================================
// SIL Fault Injection Hooks (Implementation)
// Deterministic + probabilistic fault control for SIL
// ============================================================

#include "raps/platform/sil_fault_injection.hpp"

#include <array>
#include <random>
#include <atomic>

namespace RAPS::SIL {

// We keep this simple, static, and deterministic.
// No heap usage, no exceptions.

namespace {

// One config + counters per fault point
struct FaultState {
    FaultConfig config{};
    std::atomic<uint64_t> attempts{0};
    std::atomic<uint64_t> failures{0};
};

// Enum size guard
constexpr size_t FAULT_COUNT = 5;

static std::array<FaultState, FAULT_COUNT> g_faults;

// Deterministic RNG for probabilistic faults
static std::mt19937 g_rng{0xC0FFEE};
static std::uniform_int_distribution<uint32_t> g_dist(0, 1'000'000);

inline size_t idx(FaultPoint p) {
    return static_cast<size_t>(p);
}

} // anonymous namespace

// ------------------------------------------------------------
// Public API
// ------------------------------------------------------------

void init_faults() {
    for (auto& f : g_faults) {
        f.config.probability_per_million = 0;
        f.config.forced_failure_countdown = 0;
        f.attempts.store(0);
        f.failures.store(0);
    }
}

void set_fault_config(FaultPoint p, const FaultConfig& cfg) {
    g_faults[idx(p)].config = cfg;
}

FaultConfig get_fault_config(FaultPoint p) {
    return g_faults[idx(p)].config;
}

void disable_fault(FaultPoint p) {
    g_faults[idx(p)].config.probability_per_million = 0;
    g_faults[idx(p)].config.forced_failure_countdown = 0;
}

void force_fail_next(FaultPoint p, uint32_t count) {
    g_faults[idx(p)].config.forced_failure_countdown = count;
}

bool should_fail(FaultPoint p) {
    auto& state = g_faults[idx(p)];
    state.attempts.fetch_add(1);

#if defined(RAPS_ENABLE_SIL_FAULTS) && RAPS_ENABLE_SIL_FAULTS

    // Deterministic forced failure path
    if (state.config.forced_failure_countdown > 0) {
        state.config.forced_failure_countdown--;
        state.failures.fetch_add(1);
        return true;
    }

    // Probabilistic path (per-million resolution)
    if (state.config.probability_per_million > 0) {
        uint32_t roll = g_dist(g_rng);
        if (roll < state.config.probability_per_million) {
            state.failures.fetch_add(1);
            return true;
        }
    }

#else
    (void)p;
#endif

    return false;
}

uint64_t get_attempt_count(FaultPoint p) {
    return g_faults[idx(p)].attempts.load();
}

uint64_t get_failure_count(FaultPoint p) {
    return g_faults[idx(p)].failures.load();
}

} // namespace RAPS::SIL
