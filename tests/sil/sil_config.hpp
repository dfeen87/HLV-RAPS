#pragma once

#include <cstdint>

namespace SILConfig {

// Determinism
constexpr uint32_t RANDOM_SEED = 42;

// Timing
constexpr uint32_t CYCLE_INTERVAL_MS = 50;

// Default cycles (per scenario)
constexpr uint32_t NOMINAL_CYCLES = 200;

// Failover test
constexpr uint32_t FAILOVER_AT_CYCLE = 80;

// Verbosity
constexpr bool VERBOSE_LOGGING = true;

}

