#pragma once

#include <cmath>
#include <cstdint>

// HLV Math: Power scales non-linearly with fields and with slew rate.
// warp_slew and flux_slew are expressed in units/ms.
inline float compute_power_draw_model(
    const SpacetimeModulationState& state,
    float warp_slew,
    float flux_slew) {

    float W = state.warp_field_strength;
    float Phi_g = state.gravito_flux_bias;
    float C = state.spacetime_curvature_magnitude;

    // Base power from field magnitudes:
    // MIN + P_W * W^3 + P_C * C^2 + P_Phi * |Phi_g|
    float base_power = MIN_POWER_DRAW_GW +
        (std::pow(W, 3.0f) * POWER_WARP_CUBIC_SCALAR) +
        (std::pow(C, 2.0f) * POWER_CURVATURE_QUADRATIC_SCALAR) +
        (std::fabs(Phi_g) * POWER_FLUX_LINEAR_SCALAR);

    // Quadratic (or exponent) penalty for rapid field changes (slew rate in units/ms)
    float slew_penalty =
        POWER_SLEW_PENALTY_SCALE *
        (std::pow(std::fabs(warp_slew), POWER_SLEW_PENALTY_EXPONENT) +
         std::pow(std::fabs(flux_slew), POWER_SLEW_PENALTY_EXPONENT));

    return base_power + slew_penalty;
}
