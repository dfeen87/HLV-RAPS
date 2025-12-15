#pragma once

#include <cstdint>

namespace SILConfig {

// Deterministic execution
constexpr uint32_t RANDOM_SEED = 42;

// Simulation timing
constexpr uint32_t CYCLE_INTERVAL_MS = 50;
constexpr uint32_t TOTAL_CYCLES      = 500;

// Fault injection
constexpr bool ENABLE_FAULT_INJECTION = true;
constexpr uint32_t FAULT_AT_CYCLE     = 120;

// Logging
constexpr bool VERBOSE_LOGGING = true;

}

