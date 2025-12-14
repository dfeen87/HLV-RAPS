#pragma once

#include <algorithm>
#include <cstring>
#include <cmath>

class AdvancedPropulsionControlUnit {

public:
    void update_diagnostics_and_safety(uint64_t elapsed_ms);

    void save_safe_state();
    bool restore_from_safe_state(const SpacetimeModulationState& safe_state);

    bool is_operational_state_safe() const;
    SpacetimeModulationState get_current_state() const;

    void enter_emergency_mode();
    void apply_emergency_limits(SpacetimeModulationCommand& command);

    bool initiate_emergency_spacetime_collapse();
    bool execute_controlled_shutdown();

private:
    bool is_state_safe_to_save(const SpacetimeModulationState& state) const;

private:
    SpacetimeModulationState current_propulsion_state_{};
    SpacetimeModulationState last_safe_state_{};

    uint64_t last_safe_state_timestamp_ms_{0};
    bool emergency_mode_active_{false};

    // PID integrator state
    float warp_error_integral_{0.0f};
    float warp_error_previous_{0.0f};
    float flux_error_integral_{0.0f};
    float flux_error_previous_{0.0f};
    float dilation_error_integral_{0.0f};
    float dilation_error_previous_{0.0f};
    float gravity_error_integral_{0.0f};
    float gravity_error_previous_{0.0f};
    float fluid_error_integral_{0.0f};
    float fluid_error_previous_{0.0f};

    SpacetimeModulationCommand active_spacetime_command_{};
    char active_directive_id_[64]{};
};

inline void AdvancedPropulsionControlUnit::update_diagnostics_and_safety(
    uint64_t elapsed_ms) {

    // 9. Diagnostic Metrics (computed AFTER core state changes)
    current_propulsion_state_.field_coupling_stress =
        compute_field_coupling_stress();

    current_propulsion_state_.spacetime_stability_index =
        compute_stability_index();

    current_propulsion_state_.control_authority_remaining =
        compute_control_authority();

    current_propulsion_state_.emergency_mode_active =
        emergency_mode_active_;

    // 10. State Management & Safety
    current_propulsion_state_.timestamp_ms += elapsed_ms;
    current_propulsion_state_.state_hash =
        calculate_state_hash(current_propulsion_state_);

    if (is_state_safe_to_save(current_propulsion_state_) &&
        (current_propulsion_state_.timestamp_ms -
         last_safe_state_timestamp_ms_ > 1000)) {
        save_safe_state();
    }

    if (!is_operational_state_safe() && !emergency_mode_active_) {
        enter_emergency_mode();
    }

    // Emit metrics
    PlatformHAL::metric_emit("apcu.power_draw_GW",
        current_propulsion_state_.power_draw_GW);
    PlatformHAL::metric_emit("apcu.warp_strength",
        current_propulsion_state_.warp_field_strength);
    PlatformHAL::metric_emit("apcu.flux_bias",
        current_propulsion_state_.gravito_flux_bias);
    PlatformHAL::metric_emit("apcu.curvature_mag",
        current_propulsion_state_.spacetime_curvature_magnitude);
    PlatformHAL::metric_emit("apcu.time_dilation_factor",
        current_propulsion_state_.time_dilation_factor);
    PlatformHAL::metric_emit("apcu.induced_gravity_g",
        current_propulsion_state_.induced_gravity_g);
    PlatformHAL::metric_emit("apcu.subspace_efficiency_pct",
        current_propulsion_state_.subspace_efficiency_pct);
    PlatformHAL::metric_emit("apcu.total_displacement_km",
        static_cast<float>(current_propulsion_state_.total_displacement_km));
    PlatformHAL::metric_emit("apcu.antimatter_kg",
        current_propulsion_state_.remaining_antimatter_kg);
    PlatformHAL::metric_emit("apcu.quantum_fluid_L",
        current_propulsion_state_.quantum_fluid_level);
    PlatformHAL::metric_emit("apcu.coupling_stress",
        current_propulsion_state_.field_coupling_stress);
    PlatformHAL::metric_emit("apcu.stability_index",
        current_propulsion_state_.spacetime_stability_index);
    PlatformHAL::metric_emit("apcu.control_authority",
        current_propulsion_state_.control_authority_remaining);
}

inline void AdvancedPropulsionControlUnit::save_safe_state() {
    last_safe_state_ = current_propulsion_state_;
    last_safe_state_timestamp_ms_ = current_propulsion_state_.timestamp_ms;
    PlatformHAL::metric_emit("apcu.safe_state_saved", 1.0f);
}

inline bool AdvancedPropulsionControlUnit::is_state_safe_to_save(
    const SpacetimeModulationState& state) const {

    return state.remaining_antimatter_kg >
               RAPSConfig::EMERGENCY_ANTIMATTER_RESERVE_KG &&
           state.quantum_fluid_level >
               RAPSConfig::EMERGENCY_QUANTUM_FLUID_LITERS &&
           state.field_coupling_stress <
               RAPSConfig::CRITICAL_FIELD_COUPLING_THRESHOLD &&
           state.spacetime_stability_index > 0.6f;
}

inline void AdvancedPropulsionControlUnit::enter_emergency_mode() {
    emergency_mode_active_ = true;
    current_propulsion_state_.emergency_mode_active = true;

    warp_error_integral_ = warp_error_previous_ = 0.0f;
    flux_error_integral_ = flux_error_previous_ = 0.0f;
    dilation_error_integral_ = dilation_error_previous_ = 0.0f;
    gravity_error_integral_ = gravity_error_previous_ = 0.0f;
    fluid_error_integral_ = fluid_error_previous_ = 0.0f;

    PlatformHAL::metric_emit("apcu.emergency_mode_activated", 1.0f);
}

inline bool AdvancedPropulsionControlUnit::restore_from_safe_state(
    const SpacetimeModulationState& safe_state) {

    if (!is_state_safe_to_save(safe_state)) {
        PlatformHAL::metric_emit("apcu.restore_rejected_unsafe_state", 1.0f);
        return false;
    }

    if (safe_state.remaining_antimatter_kg >
            current_propulsion_state_.remaining_antimatter_kg ||
        safe_state.quantum_fluid_level >
            current_propulsion_state_.quantum_fluid_level) {

        PlatformHAL::metric_emit(
            "apcu.restore_rejected_insufficient_resources", 1.0f);
        return false;
    }

    current_propulsion_state_.warp_field_strength =
        safe_state.warp_field_strength;
    current_propulsion_state_.gravito_flux_bias =
        safe_state.gravito_flux_bias;
    current_propulsion_state_.spacetime_curvature_magnitude =
        safe_state.spacetime_curvature_magnitude;
    current_propulsion_state_.time_dilation_factor =
        safe_state.time_dilation_factor;
    current_propulsion_state_.induced_gravity_g =
        safe_state.induced_gravity_g;

    warp_error_integral_ = warp_error_previous_ = 0.0f;
    flux_error_integral_ = flux_error_previous_ = 0.0f;
    dilation_error_integral_ = dilation_error_previous_ = 0.0f;
    gravity_error_integral_ = gravity_error_previous_ = 0.0f;

    if (emergency_mode_active_) {
        emergency_mode_active_ = false;
        current_propulsion_state_.emergency_mode_active = false;
        PlatformHAL::metric_emit("apcu.emergency_mode_deactivated", 1.0f);
    }

    PlatformHAL::metric_emit("apcu.state_restored", 1.0f);
    return true;
}

inline SpacetimeModulationState
AdvancedPropulsionControlUnit::get_current_state() const {
    return current_propulsion_state_;
}

inline bool AdvancedPropulsionControlUnit::is_operational_state_safe() const {

    bool safe = true;

    if (current_propulsion_state_.remaining_antimatter_kg <
        RAPSConfig::CRITICAL_ANTIMATTER_KG) {
        PlatformHAL::metric_emit("apcu.safety_fuel_critical", 1.0f);
        safe = false;
    }

    if (current_propulsion_state_.quantum_fluid_level <
        RAPSConfig::CRITICAL_QUANTUM_FLUID_LITERS) {
        PlatformHAL::metric_emit("apcu.safety_quantum_fluid_critical", 1.0f);
        safe = false;
    }

    if (current_propulsion_state_.power_draw_GW >
        MAX_SYSTEM_POWER_DRAW_GW * 0.98f) {
        PlatformHAL::metric_emit("apcu.safety_power_critical", 1.0f);
        safe = false;
    }

    if (current_propulsion_state_.spacetime_curvature_magnitude >
        MAX_SPACETIME_CURVATURE_MAGNITUDE * 0.98f) {
        PlatformHAL::metric_emit("apcu.safety_curvature_critical", 1.0f);
        safe = false;
    }

    if (current_propulsion_state_.time_dilation_factor >
        MAX_TIME_DILATION_FACTOR * 0.98f) {
        PlatformHAL::metric_emit("apcu.safety_time_dilation_critical", 1.0f);
        safe = false;
    }

    if (std::fabs(current_propulsion_state_.induced_gravity_g) >
        MAX_INDUCED_GRAVITY_G * 0.98f) {
        PlatformHAL::metric_emit("apcu.safety_gravity_critical", 1.0f);
        safe = false;
    }

    if (current_propulsion_state_.warp_field_strength >
            MAX_WARP_FIELD_STRENGTH * 1.01f ||
        std::fabs(current_propulsion_state_.gravito_flux_bias) >
            MAX_GRAVITO_FLUX_BIAS * 1.01f) {

        PlatformHAL::metric_emit("apcu.safety_field_oob_critical", 1.0f);
        safe = false;
    }

    if (current_propulsion_state_.field_coupling_stress >
        RAPSConfig::CRITICAL_FIELD_COUPLING_THRESHOLD) {
        PlatformHAL::metric_emit("apcu.safety_coupling_stress_critical", 1.0f);
        safe = false;
    }

    if (current_propulsion_state_.spacetime_stability_index < 0.3f) {
        PlatformHAL::metric_emit("apcu.safety_stability_critical", 1.0f);
        safe = false;
    }

    if (current_propulsion_state_.control_authority_remaining < 0.1f) {
        PlatformHAL::metric_emit("apcu.safety_control_authority_low", 1.0f);
        safe = false;
    }

    return safe;
}
