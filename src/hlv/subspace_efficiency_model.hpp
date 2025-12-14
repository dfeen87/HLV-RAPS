#pragma once

#include <algorithm>
#include <cmath>

// HLV Math: Efficiency scales with W^2/P, Gaussian penalty by power draw,
// fluid depletion penalty, and stability bonus.
inline float compute_subspace_efficiency_model(
    const SpacetimeModulationState& state) {

    float W = state.warp_field_strength;
    float P = state.power_draw_GW;

    // 1) Base Efficiency: scales with W^2 / P (only valid when both are meaningful)
    float base_efficiency =
        (W > 0.1f && P > MIN_POWER_DRAW_GW)
            ? (EFFICIENCY_WARP_QUADRATIC_SCALAR * std::pow(W, 2.0f) / P)
            : 0.0f;

    // 2) Power Penalty (Gaussian curve): efficiency peaks at a specific power level
    float power_diff = state.power_draw_GW - EFFICIENCY_POWER_PEAK_GW;
    float power_penalty = std::exp(
        -0.5f * std::pow(power_diff / EFFICIENCY_POWER_VARIANCE_GW, 2.0f)
    );

    // 3) Fluid Penalty: penalize low fluid level (ratio^exponent)
    float fluid_ratio = state.quantum_fluid_level / INITIAL_QUANTUM_FLUID_LITERS;
    float fluid_modulation = std::pow(
        std::min(1.0f, fluid_ratio),
        EFFICIENCY_FLUID_EXPONENT
    );

    // 4) Stability Bonus: quadratic boost for high stability (max 10% boost)
    float stability_bonus =
        state.spacetime_stability_index *
        state.spacetime_stability_index *
        10.0f;

    float efficiency =
        base_efficiency * power_penalty * fluid_modulation +
        stability_bonus;

    return std::max(
        0.0f,
        std::min(RAPSConfig::MAX_SUBSPACE_EFFICIENCY, efficiency)
    );
}
