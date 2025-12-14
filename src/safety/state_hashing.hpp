#pragma once

#include <array>

// Deterministic state hashing for audit and rollback
inline Hash256 calculate_state_hash(
    const SpacetimeModulationState& state) {

    std::array<float, 10> hash_input = {
        state.power_draw_GW,
        state.warp_field_strength,
        state.gravito_flux_bias,
        state.spacetime_curvature_magnitude,
        state.time_dilation_factor,
        state.induced_gravity_g,
        state.subspace_efficiency_pct,
        static_cast<float>(state.total_displacement_km),
        state.remaining_antimatter_kg,
        state.quantum_fluid_level
    };

    return PlatformHAL::sha256(
        hash_input.data(),
        sizeof(hash_input)
    );
}
