#pragma once
#include <cmath>
#include "hlv/spacetime_modulation_types.hpp"

inline bool is_finite_and_valid(float v) {
    return std::isfinite(v);
}

inline bool sanitize_spacetime_state(const SpacetimeModulationState& s) {
    return is_finite_and_valid(s.warp_field_strength)
        && is_finite_and_valid(s.gravito_flux_bias)
        && is_finite_and_valid(s.spacetime_curvature_magnitude)
        && is_finite_and_valid(s.time_dilation_factor)
        && is_finite_and_valid(s.induced_gravity_g)
        && is_finite_and_valid(s.spacetime_stability_index)
        && is_finite_and_valid(s.power_draw_GW)
        && is_finite_and_valid(s.remaining_antimatter_kg)
        && is_finite_and_valid(s.quantum_fluid_level)
        && is_finite_and_valid(s.field_coupling_stress)
        && is_finite_and_valid(s.control_authority_remaining);
}
