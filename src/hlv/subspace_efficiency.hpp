#pragma once

#include <algorithm>
#include <cmath>

// Subspace Efficiency Calculation
inline float compute_subspace_efficiency(
    const SpacetimeModulationState& state,
    float base_efficiency,
    float power_penalty,
    float fluid_modulation) {

    // Stability Bonus: Quadratic boost for high stability (max 10%)
    float stability_bonus =
        state.spacetime_stability_index *
        state.spacetime_stability_index * 10.0f;

    float efficiency =
        base_efficiency * power_penalty * fluid_modulation +
        stability_bonus;

    return std::max(
        0.0f,
        std::min(
            RAPSConfig::MAX_SUBSPACE_EFFICIENCY,
            efficiency
        )
    );
}
