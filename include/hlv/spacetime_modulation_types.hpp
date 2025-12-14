#pragma once

#include <cstdint>
#include "Hash256.hpp"

// =====================================================
// Core Data Structures
// =====================================================

struct SpacetimeModulationState {
    float power_draw_GW{0.0f};

    float warp_field_strength{0.0f};
    float gravito_flux_bias{0.0f};
    float spacetime_curvature_magnitude{0.0f};

    float time_dilation_factor{1.0f};
    float induced_gravity_g{0.0f};

    float subspace_efficiency_pct{0.0f};
    double total_displacement_km{0.0};

    float remaining_antimatter_kg{0.0f};
    float quantum_fluid_level{0.0f};

    float field_coupling_stress{0.0f};
    float spacetime_stability_index{1.0f};
    float control_authority_remaining{1.0f};

    bool emergency_mode_active{false};

    uint64_t timestamp_ms{0};
    Hash256 state_hash{};
};

struct SpacetimeModulationCommand {
    float target_warp_field_strength{0.0f};
    float target_gravito_flux_bias{0.0f};
    float target_time_dilation_factor{1.0f};
    float target_artificial_gravity_g{0.0f};

    float target_quantum_fluid_flow_rate{0.0f}; // L/s
    float target_power_budget_GW{0.0f};

    bool enable_emergency_damping{false};
    bool enable_resonance_suppression{false};
    bool enable_time_dilation_coupling{true};
};
