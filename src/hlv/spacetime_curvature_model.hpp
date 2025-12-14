#pragma once

#include <algorithm>
#include <cmath>

// HLV Math: Curvature as a non-linear function of fields
inline float compute_spacetime_curvature(
    const SpacetimeModulationState& state) {

    float W = state.warp_field_strength;
    float Phi_g = state.gravito_flux_bias;

    float curvature =
        (std::pow(W, 3.0f) * WARP_CURVATURE_CUBIC_SCALAR) +
        (0.5f * std::pow(std::fabs(Phi_g), 2.0f) *
         FLUX_CURVATURE_QUADRATIC_SCALAR);

    return std::max(
        0.0f,
        std::min(
            MAX_SPACETIME_CURVATURE_MAGNITUDE,
            curvature
        )
    );
}
