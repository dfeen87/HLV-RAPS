#pragma once

#include <algorithm>

// Derived Spacetime Curvature Dynamics (Section 4)
// Curvature evolves gradually toward the value derived from field states.
inline void update_spacetime_curvature(
    SpacetimeModulationState& state,
    float dt_s) {

    // Compute instantaneous curvature from current field configuration
    float new_curvature = compute_spacetime_curvature();

    // Spacetime inertia: gradual convergence toward derived curvature
    float curvature_error =
        new_curvature - state.spacetime_curvature_magnitude;

    float curvature_change =
        curvature_error * 0.1f * dt_s;

    state.spacetime_curvature_magnitude += curvature_change;

    // Hard physical bounds
    state.spacetime_curvature_magnitude = std::max(
        0.0f,
        std::min(
            MAX_SPACETIME_CURVATURE_MAGNITUDE,
            state.spacetime_curvature_magnitude
        )
    );
}
