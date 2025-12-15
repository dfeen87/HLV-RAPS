#pragma once

// ============================================================================
// SIL Fault Injection Configuration
// ----------------------------------------------------------------------------
// Purpose:
//  - Single authoritative location for SIL fault injection controls
//  - Keeps PlatformHAL and flight code free of scattered #ifdef logic
//  - Enables deterministic, reproducible CI testing
//
// Build usage examples:
//  -DRAPS_ENABLE_SIL_FAULTS=1
//  -DRAPS_SIL_FAULTS_DETERMINISTIC=1
//  -DRAPS_SIL_FAULTS_ALLOW_PROBABILISTIC=0
//
// IMPORTANT:
//  - This header MUST NOT be included by flight-only builds
//  - All usage must remain compile-time gated
// ============================================================================

// ----------------------------------------------------------------------------
// Master enable for SIL fault injection
// ----------------------------------------------------------------------------
#ifndef RAPS_ENABLE_SIL_FAULTS
#define RAPS_ENABLE_SIL_FAULTS 0
#endif

// ----------------------------------------------------------------------------
// Deterministic fault injection preference
// ----------------------------------------------------------------------------
// When enabled, faults should be:
//  - one-shot
//  - sequence-driven
//  - reproducible across runs
#ifndef RAPS_SIL_FAULTS_DETERMINISTIC
#define RAPS_SIL_FAULTS_DETERMINISTIC 1
#endif

// ----------------------------------------------------------------------------
// Probabilistic fault allowance
// ----------------------------------------------------------------------------
// Set to 0 in CI for strict determinism.
// Set to 1 for stress/fuzz-style SIL runs.
#ifndef RAPS_SIL_FAULTS_ALLOW_PROBABILISTIC
#define RAPS_SIL_FAULTS_ALLOW_PROBABILISTIC 1
#endif

// ----------------------------------------------------------------------------
// Default probabilistic fault rates
// (Only used when probabilistic faults are enabled)
// ----------------------------------------------------------------------------
#ifndef RAPS_SIL_PROB_FLASH_WRITE_FAIL
#define RAPS_SIL_PROB_FLASH_WRITE_FAIL 0.005f   // 0.5%
#endif

#ifndef RAPS_SIL_PROB_FLASH_READ_FAIL
#define RAPS_SIL_PROB_FLASH_READ_FAIL  0.001f   // 0.1%
#endif

#ifndef RAPS_SIL_PROB_DOWNLINK_FAIL
#define RAPS_SIL_PROB_DOWNLINK_FAIL    0.001f   // 0.1%
#endif

#ifndef RAPS_SIL_PROB_ACTUATOR_FAIL
#define RAPS_SIL_PROB_ACTUATOR_FAIL    0.002f   // 0.2%
#endif

// ----------------------------------------------------------------------------
// Actuator latency model (seconds)
// ----------------------------------------------------------------------------
// Used by SIL actuator stubs to simulate timing behavior.
// These values are intentionally conservative.
#ifndef RAPS_SIL_ACTUATOR_LAT_MIN_S
#define RAPS_SIL_ACTUATOR_LAT_MIN_S 0.003f
#endif

#ifndef RAPS_SIL_ACTUATOR_LAT_MAX_S
#define RAPS_SIL_ACTUATOR_LAT_MAX_S 0.020f
#endif

// ----------------------------------------------------------------------------
// Compile-time sanity checks
// ----------------------------------------------------------------------------
#if RAPS_SIL_ACTUATOR_LAT_MAX_S < RAPS_SIL_ACTUATOR_LAT_MIN_S
#error "RAPS_SIL_ACTUATOR_LAT_MAX_S must be >= RAPS_SIL_ACTUATOR_LAT_MIN_S"
#endif

#if (RAPS_SIL_PROB_FLASH_WRITE_FAIL < 0.0f) || (RAPS_SIL_PROB_FLASH_WRITE_FAIL > 1.0f)
#error "RAPS_SIL_PROB_FLASH_WRITE_FAIL must be in [0.0, 1.0]"
#endif

#if (RAPS_SIL_PROB_ACTUATOR_FAIL < 0.0f) || (RAPS_SIL_PROB_ACTUATOR_FAIL > 1.0f)
#error "RAPS_SIL_PROB_ACTUATOR_FAIL must be in [0.0, 1.0]"
#endif

// ----------------------------------------------------------------------------
// Documentation note:
//
// This header intentionally contains:
//  - NO runtime state
//  - NO global variables
//  - NO function definitions
//
// All behavior is implemented in PlatformHAL.cpp or SIL harnesses,
// using these constants as compile-time policy.
// ----------------------------------------------------------------------------
