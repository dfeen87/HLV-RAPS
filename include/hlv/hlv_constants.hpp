#pragma once

#include <cstdint>

// =====================================================
// Core Physical & System Constants
// =====================================================

// --- Initial Resources ---
static constexpr float INITIAL_ANTIMATTER_KG = 1000.0f;
static constexpr float INITIAL_QUANTUM_FLUID_LITERS = 10000.0f;
static constexpr float MIN_POWER_DRAW_GW = 0.5f;

// --- Field Limits ---
static constexpr float MAX_WARP_FIELD_STRENGTH = 10.0f;      // Unitless
static constexpr float MAX_GRAVITO_FLUX_BIAS = 5.0f;         // Unitless
static constexpr float MAX_TIME_DILATION_FACTOR = 50.0f;    // Ship time / external time
static constexpr float MAX_INDUCED_GRAVITY_G = 2.5f;         // G's
static constexpr float MAX_SYSTEM_POWER_DRAW_GW = 500.0f;
static constexpr float MAX_SPACETIME_CURVATURE_MAGNITUDE = 100.0f;

// =====================================================
// HLV Propulsion Math Constants
// =====================================================

// --- Curvature & Dilation Physics ---
static constexpr float WARP_CURVATURE_CUBIC_SCALAR = 0.8f;
static constexpr float FLUX_CURVATURE_QUADRATIC_SCALAR = 0.4f;
static constexpr float CURVATURE_TIME_DILATION_EXPONENT_BASE = 0.05f;
static constexpr float FLUID_LEVEL_DAMPING_FACTOR = 0.1f;

// --- Gravity Physics ---
static constexpr float FLUX_GRAVITY_BASE_SCALAR = 0.5f;
static constexpr float WARP_GRAVITY_MODULATION_SCALAR = 0.1f;

// --- Coupling Stress & Stability ---
static constexpr float COUPLING_STRESS_EXPONENT_SCALAR = 0.2f;
static constexpr float STABILITY_STRESS_QUADRATIC_SCALAR = 0.5f;
static constexpr float STABILITY_CURVATURE_CUBIC_SCALAR = 0.005f;

// --- Power & Efficiency ---
static constexpr float POWER_WARP_CUBIC_SCALAR = 0.2f;
static constexpr float POWER_CURVATURE_QUADRATIC_SCALAR = 0.01f;
static constexpr float POWER_FLUX_LINEAR_SCALAR = 5.0f;

static constexpr float POWER_SLEW_PENALTY_SCALE = 1000.0f;
static constexpr float POWER_SLEW_PENALTY_EXPONENT = 2.0f;

static constexpr float EFFICIENCY_WARP_QUADRATIC_SCALAR = 10.0f;
static constexpr float EFFICIENCY_FLUID_EXPONENT = 0.5f;
static constexpr float EFFICIENCY_POWER_PEAK_GW = 250.0f;
static constexpr float EFFICIENCY_POWER_VARIANCE_GW = 50.0f;

// --- Resource Consumption & Displacement ---
static constexpr float ANTIMATTER_BURN_RATE_GW_TO_KG_PER_MS = 1.0e-12f;
static constexpr float QUANTUM_FLUID_BASE_CONSUMPTION_RATE = 1.0e-5f;
static constexpr float QUANTUM_FLUID_CONSUMPTION_CURVATURE_EXPONENT = 1.5f;
static constexpr float QUANTUM_FLUID_CONSUMPTION_PER_CURVATURE_UNIT_MS = 5.0e-6f;

static constexpr float WARP_TO_DISPLACEMENT_FACTOR_KM_PER_S = 1000.0f;

// =====================================================
// PID Control Parameters (reference values)
// =====================================================

static constexpr float WARP_KP = 0.5f;
static constexpr float WARP_KI = 0.001f;
static constexpr float WARP_KD = 0.1f;
static constexpr float WARP_INTEGRAL_LIMIT = 50.0f;

static constexpr float FLUX_KP = 0.8f;
static constexpr float FLUX_KI = 0.005f;
static constexpr float FLUX_KD = 0.15f;
static constexpr float FLUX_INTEGRAL_LIMIT = 20.0f;

static constexpr float DILATION_KP = 0.2f;
static constexpr float DILATION_KI = 0.0005f;
static constexpr float DILATION_KD = 0.05f;

static constexpr float GRAVITY_KP = 0.4f;
static constexpr float GRAVITY_KI = 0.002f;
static constexpr float GRAVITY_KD = 0.1f;

// =====================================================
// Control Response & Emergency Parameters
// =====================================================

static constexpr float WARP_FIELD_RESPONSE_RATE_PER_MS = 0.01f;
static constexpr float GRAVITO_FLUX_RESPONSE_RATE_PER_MS = 0.01f;
static constexpr float TIME_DILATION_RESPONSE_RATE_PER_MS = 0.005f;
static constexpr float GRAVITY_RESPONSE_RATE_PER_MS = 0.005f;

static constexpr float EMERGENCY_RESPONSE_DAMPING_FACTOR = 0.2f;

// --- Resonance Detection ---
static constexpr uint32_t RESONANCE_SAMPLE_COUNT = 50;
