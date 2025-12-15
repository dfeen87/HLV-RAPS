#pragma once

// ============================================================
// SIL Fault Injection Config
// ------------------------------------------------------------
// Purpose:
// - Central place for SIL fault injection compile-time controls
// - Keeps PlatformHAL implementation clean and deterministic-friendly
//
// Build usage:
//   -DRAPS_ENABLE_SIL_FAULTS=1        (enable SIL faults)
//   -DRAPS_SIL_FAULTS_DETERMINISTIC=1 (prefer deterministic injector when set)
// ============================================================

#ifndef RAPS_ENABLE_SIL_FAULTS
#define RAPS_ENABLE_SIL_FAULTS 0
#endif

#ifndef RAPS_SIL_FAULTS_DETERMINISTIC
#define RAPS_SIL_FAULTS_DETERMINISTIC 1
#endif

// If you want to also keep probabilistic faults enabled, leave this at 1.
// If you want deterministic-only faulting in CI, set to 0.
#ifndef RAPS_SIL_FAULTS_ALLOW_PROBABILISTIC
#define RAPS_SIL_FAULTS_ALLOW_PROBABILISTIC 1
#endif

// Default probabilities (used only if probabilistic faults are enabled)
#ifndef RAPS_SIL_PROB_FLASH_WRITE_FAIL
#define RAPS_SIL_PROB_FLASH_WRITE_FAIL 0.005f
#endif

#ifndef RAPS_SIL_PROB_FLASH_READ_FAIL
#define RAPS_SIL_PROB_FLASH_READ_FAIL 0.001f
#endif

#ifndef RAPS_SIL_PROB_DOWNLINK_FAIL
#define RAPS_SIL_PROB_DOWNLINK_FAIL 0.001f
#endif

#ifndef RAPS_SIL_PROB_ACTUATOR_FAIL
#define RAPS_SIL_PROB_ACTUATOR_FAIL 0.002f
#endif

// Default actuator latency model (seconds)
#ifndef RAPS_SIL_ACTUATOR_LAT_MIN_S
#define RAPS_SIL_ACTUATOR_LAT_MIN_S 0.003f
#endif

#ifndef RAPS_SIL_ACTUATOR_LAT_MAX_S
#define RAPS_SIL_ACTUATOR_LAT_MAX_S 0.020f
#endif
