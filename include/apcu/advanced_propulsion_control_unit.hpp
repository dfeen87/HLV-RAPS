#pragma once

#include <array>
#include <cstdint>
#include <cstring>

// Core types / constants should be declared elsewhere and included here.
#include "SpacetimeModulationTypes.hpp"   // SpacetimeModulationState, Command, constants
#include "PlatformHAL.hpp"               // now_ms(), metric_emit(), sha256(), etc.
#include "Hash256.hpp"

class AdvancedPropulsionControlUnit {
public:
    AdvancedPropulsionControlUnit() = default;

    void init();
    void update_internal_state(uint32_t elapsed_ms);

    bool receive_and_execute_spacetime_command(
        const SpacetimeModulationCommand& command,
        const char* directive_id);

    SpacetimeModulationState get_current_state() const;

    bool execute_controlled_shutdown();
    bool initiate_emergency_spacetime_collapse();
    bool restore_from_safe_state(const SpacetimeModulationState& safe_state);

private:
    // Safety & resilience (orchestration-owned)
    bool is_operational_state_safe() const;
    bool is_state_safe_to_save(const SpacetimeModulationState& state) const;

    void save_safe_state();
    void enter_emergency_mode();
    void apply_emergency_limits(SpacetimeModulationCommand& command);

private:
    // --- State ---
    SpacetimeModulationState current_propulsion_state_{};
    SpacetimeModulationCommand active_spacetime_command_{};
    char active_directive_id_[64]{};

    SpacetimeModulationState last_safe_state_{};
    uint64_t last_safe_state_timestamp_ms_{0};

    bool emergency_mode_active_{false};

    // --- PID State ---
    float warp_error_integral_{0.0f};
    float warp_error_previous_{0.0f};

    float flux_error_integral_{0.0f};
    float flux_error_previous_{0.0f};

    float dilation_error_integral_{0.0f};
    float dilation_error_previous_{0.0f};

    float gravity_error_integral_{0.0f};
    float gravity_error_previous_{0.0f};

    float fluid_error_integral_{0.0f};    // Placeholder for future fluid control
    float fluid_error_previous_{0.0f};

    // --- Resonance Detection (stateful history) ---
    std::array<float, RESONANCE_SAMPLE_COUNT> field_coupling_history_{};
    uint32_t coupling_history_index_{0};
};
