#pragma once

#include <algorithm>
#include <cmath>

// Computes capability scaling based on remaining resources
inline float compute_capability_scale(
    const SpacetimeModulationState& state) {

    float scale = 1.0f;

    // Antimatter constraint
    if (state.remaining_antimatter_kg <
        INITIAL_ANTIMATTER_KG * 0.1f) {

        scale *= std::pow(
            state.remaining_antimatter_kg /
            (INITIAL_ANTIMATTER_KG * 0.1f),
            2.0f
        );
        scale = std::max(0.05f, scale);
    }

    // Quantum fluid constraint
    if (state.quantum_fluid_level <
        INITIAL_QUANTUM_FLUID_LITERS * 0.1f) {

        scale *= std::pow(
            state.quantum_fluid_level /
            (INITIAL_QUANTUM_FLUID_LITERS * 0.1f),
            2.0f
        );
        scale = std::max(0.05f, scale);
    }

    return std::min(1.0f, scale);
}
