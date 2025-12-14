#pragma once

#include <cmath>

// HLV Math: Time dilation derived from curvature and fluid availability
inline float compute_derived_time_dilation(
    const SpacetimeModulationState& state) {

    float C = state.spacetime_curvature_magnitude;
    float L_fluid = state.quantum_fluid_level;

    float fluid_ratio =
        L_fluid / INITIAL_QUANTUM_FLUID_LITERS;

    float fluid_efficiency_mod =
        1.0f - std::exp(
            -fluid_ratio * FLUID_LEVEL_DAMPING_FACTOR
        );

    float base_dilation_from_C =
        1.0f +
        (std::exp(
            C * CURVATURE_TIME_DILATION_EXPONENT_BASE
        ) - 1.0f);

    return 1.0f +
        ((base_dilation_from_C - 1.0f) *
         fluid_efficiency_mod);
}
