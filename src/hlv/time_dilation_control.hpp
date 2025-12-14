#pragma once

#include <algorithm>

// Time Dilation Control (Section 5)
// Supports active PID-based control or passive physics-derived mode.
inline void update_time_dilation_control(
    SpacetimeModulationState& state,
    const SpacetimeModulationCommand& command,
    float capability_scale,
    float response_scale,
    float& dilation_error_integral,
    float& dilation_error_previous,
    uint64_t elapsed_ms) {

    if (command.enable_time_dilation_coupling) {
        // Active control mode: PID regulation via field modulation
        float dilation_error =
            command.target_time_dilation_factor -
            state.time_dilation_factor;

        float dilation_pid_output = compute_pid_output(
            dilation_error,
            dilation_error_integral,
            dilation_error_previous,
            DILATION_KP,
            DILATION_KI,
            DILATION_KD,
            0.5f, // Integral limit
            elapsed_ms
        );

        float dilation_change =
            dilation_pid_output * capability_scale * response_scale;

        float max_delta =
            TIME_DILATION_RESPONSE_RATE_PER_MS *
            static_cast<float>(elapsed_ms);

        dilation_change = std::max(
            -max_delta,
            std::min(max_delta, dilation_change)
        );

        state.time_dilation_factor += dilation_change;
    } else {
        // Passive mode: derived purely from curvature and fluid state
        state.time_dilation_factor =
            compute_derived_time_dilation();
    }

    // Hard physical bounds
    state.time_dilation_factor = std::max(
        1.0f,
        std::min(
            MAX_TIME_DILATION_FACTOR,
            state.time_dilation_factor
        )
    );
}
