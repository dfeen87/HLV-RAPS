#pragma once

#include <cmath>

// HLV Math: Gravity is primarily linear with flux,
// modulated by warp field magnitude.
inline float compute_derived_gravity(
    const SpacetimeModulationState& state) {

    float W = state.warp_field_strength;
    float Phi_g = state.gravito_flux_bias;

    // G = Phi_g * G_Phi * (1 + W * G_W)
    float gravity =
        Phi_g *
        FLUX_GRAVITY_BASE_SCALAR *
        (1.0f + W * WARP_GRAVITY_MODULATION_SCALAR);

    return gravity; // Bounded by caller
}
