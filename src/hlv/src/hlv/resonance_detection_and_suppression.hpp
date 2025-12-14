#pragma once

#include <algorithm>

// Resonance Detection & Suppression (Section 3)
// Detects unstable resonance conditions and applies suppression
// before field changes are committed.
inline void update_resonance_and_apply_fields(
    SpacetimeModulationState& state,
    const SpacetimeModulationCommand& command,
    float& warp_change_request,
    float& flux_change_request) {

    if (command.enable_resonance_suppression &&
        detect_resonance_instability()) {

        apply_resonance_suppression(
            warp_change_request,
            flux_change_request
        );
    }

    // Apply field changes
    state.warp_field_strength += warp_change_request;
    state.warp_field_strength = std::max(
        0.0f,
        std::min(
            MAX_WARP_FIELD_STRENGTH,
            state.warp_field_strength
        )
    );

    state.gravito_flux_bias += flux_change_request;
    state.gravito_flux_bias = std::max(
        -MAX_GRAVITO_FLUX_BIAS,
        std::min(
            MAX_GRAVITO_FLUX_BIAS,
            state.gravito_flux_bias
        )
    );
}
