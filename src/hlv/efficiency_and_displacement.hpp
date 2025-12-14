#pragma once

inline void update_efficiency_and_displacement(
    SpacetimeModulationState& state,
    float dt_s) {

    // Efficiency & Displacement (Section 8)
    state.subspace_efficiency_pct =
        compute_subspace_efficiency(state);

    state.total_displacement_km +=
        state.warp_field_strength *
        (state.subspace_efficiency_pct / 100.0f) *
        WARP_TO_DISPLACEMENT_FACTOR_KM_PER_S *
        dt_s;
}
