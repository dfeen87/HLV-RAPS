#pragma once

#include <algorithm>
#include <cmath>

// Stability Index Calculation
inline float compute_stability_index(
    const SpacetimeModulationState& state,
    const SpacetimeModulationCommand& command) {

    float C = state.spacetime_curvature_magnitude;
    float Sigma = state.field_coupling_stress;

    // Penalties
    float curvature_penalty =
        STABILITY_CURVATURE_CUBIC_SCALAR * std::pow(C, 3.0f);

    float stress_penalty =
        STABILITY_STRESS_QUADRATIC_SCALAR * std::pow(Sigma, 2.0f);

    float base_stability =
        1.0f - curvature_penalty - stress_penalty;

    // Active control mismatch penalty
    float warp_mismatch =
        std::fabs(state.warp_field_strength -
                  command.target_warp_field_strength) /
        MAX_WARP_FIELD_STRENGTH;

    float flux_mismatch =
        std::fabs(state.gravito_flux_bias -
                  command.target_gravito_flux_bias) /
        MAX_GRAVITO_FLUX_BIAS;

    float mismatch_penalty =
        (warp_mismatch + flux_mismatch) * 0.1f;

    float final_stability =
        base_stability - mismatch_penalty;

    return std::max(
        0.0f,
        std::min(1.0f, final_stability)
    );
}

// Control Authority Metric
inline float compute_control_authority(
    const SpacetimeModulationState& state,
    float resource_capability_scale) {

    float warp_margin =
        1.0f -
        (state.warp_field_strength / MAX_WARP_FIELD_STRENGTH);

    float flux_margin =
        1.0f -
        (std::fabs(state.gravito_flux_bias) / MAX_GRAVITO_FLUX_BIAS);

    float power_margin =
        1.0f -
        (state.power_draw_GW / MAX_SYSTEM_POWER_DRAW_GW);

    float stability_factor =
        state.spacetime_stability_index;

    float authority =
        (resource_capability_scale * 0.3f) +
        (stability_factor * 0.3f) +
        ((warp_margin + flux_margin + power_margin) / 3.0f * 0.4f);

    return std::max(
        0.0f,
        std::min(1.0f, authority)
    );
}
