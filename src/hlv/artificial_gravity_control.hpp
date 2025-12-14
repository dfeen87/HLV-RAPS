#pragma once

#include <algorithm>

// Artificial Gravity Control (Section 6)
// Full PID loop with rate limiting and hard bounds.
inline void update_artificial_gravity_control(
    SpacetimeModulationState& state,
    const SpacetimeModulationCommand& command,
    float capability_scale,
    float response_scale,
    float& gravity_error_integral,
    float& gravity_error_previous,
    uint64_t elapsed_ms) {

    float gravity_error =
        command.target_artificial_gravity_g -
        state.induced_gravity_g;

    float gravity_pid_output = compute_pid_output(
        gravity_error,
        gravity_error_integral,
        gravity_error_previous,
        GRAVITY_KP,
        GRAVITY_KI,
        GRAVITY_KD,
        0.5f,
        elapsed_ms
    );

    float gravity_change =
        gravity_pid_output * capability_scale * response_scale;

    gravity_change = std::max(
        -GRAVITY_RESPONSE_RATE_PER_MS * elapsed_ms,
        std::min(
            GRAVITY_RESPONSE_RATE_PER_MS * elapsed_ms,
            gravity_change
        )
    );

    // Gravity is primarily derived from flux; PID fine-tunes the result
    float derived_gravity = compute_derived_gravity();

    state.induced_gravity_g =
        derived_gravity + gravity_change;

    state.induced_gravity_g = std::max(
        -MAX_INDUCED_GRAVITY_G,
        std::min(
            MAX_INDUCED_GRAVITY_G,
            state.induced_gravity_g
        )
    );
}
