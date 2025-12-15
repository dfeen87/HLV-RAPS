#pragma once
// ============================================================
// SIL Fault Injection Hooks (Header)
// - Optional compile-time feature
// - Deterministic knobs for tests + CI
//
// Enable with:
//   -DRAPS_ENABLE_SIL_FAULTS=1
// ============================================================

#include <cstdint>

namespace RAPS::SIL {

// High-level fault categories used by PlatformHAL stubs.
// Keep this small and stable: tests depend on it.
enum class FaultPoint : uint8_t {
    FLASH_WRITE = 0,
    FLASH_READ  = 1,
    DOWNLINK    = 2,
    ACTUATOR    = 3,
    METRICS     = 4,
};

// Controls for deterministic behavior in SIL.
// - If forced_failure_countdown > 0: next N should_fail() calls return true,
//   then it returns to probabilistic mode.
// - Probability is expressed in "per million" to avoid floats.
struct FaultConfig {
    uint32_t probability_per_million = 0;   // 0 => disabled
    uint32_t forced_failure_countdown = 0;  // deterministic "trip" mode
};

// Initialize with safe defaults (faults off).
void init_faults();

// Set / get configuration for a fault point.
void set_fault_config(FaultPoint p, const FaultConfig& cfg);
FaultConfig get_fault_config(FaultPoint p);

// Convenience helpers
void disable_fault(FaultPoint p);
void force_fail_next(FaultPoint p, uint32_t count = 1);

// Decision hook used by PlatformHAL stubs.
// Returns true if the operation should fail.
bool should_fail(FaultPoint p);

// For observability in tests.
uint64_t get_attempt_count(FaultPoint p);
uint64_t get_failure_count(FaultPoint p);

} // namespace RAPS::SIL
