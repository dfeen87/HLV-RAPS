#pragma once

#include <algorithm>
#include <cmath>

// HLV Math: Field coupling stress grows exponentially with
// combined high fields and time dilation deviation,
// penalized by low stability.
inline float compute_field_coupling_stress(
    const SpacetimeModulationState& state) {

    float W = state.warp_field_strength;
    float Phi_g = state.gravito_flux_bias;
    float D = state.time_dilation_factor;
    float S = state.spacetime_stability_index;

    // Prevent division by zero when stability is very low
    float stability_penalty_factor =
        (S > 0.05f) ? (1.0f / S) : (1.0f / 0.05f);

    // Sigma = exp((W * |Phi_g| * (D - 1) * k) / S) - 1
    float stress_term =
        W *
        std::fabs(Phi_g) *
        (D - 1.0f) *
        COUPLING_STRESS_EXPONENT_SCALAR;

    float coupling_stress =
        std::exp(stress_term * stability_penalty_factor) - 1.0f;

    return std::max(0.0f, coupling_stress);
}
