#pragma once

#include <algorithm>

// Warp Field Control (Section 1)
// Full PID control with rate limiting.
inline float compute_warp_field_change(
    const SpacetimeModulationState& state,
    const SpacetimeModulationCommand& command,
    float capability_scale,
    float response_scale,
    float& warp_error_integral,
    float& warp_error_previous,
    uint64_t elapsed_ms) {

    float warp_error =
        command.target_warp_field_strength -
        state.warp_field_strength;

    float warp_pid_output = compute_pid_output(
        warp_error,
        warp_error_integral,
        warp_error_previous,
        WARP_KP,
        WARP_KI,
        WARP_KD,
        WARP_INTEGRAL_LIMIT,
        elapsed_ms
    );

    float warp_change_request =
        warp_pid_output * capability_scale * response_scale;

    float max_delta =
        WARP_FIELD_RESPONSE_RATE_PER_MS *
        static_cast<float>(elapsed_ms);

    warp_change_request = std::max(
        -max_delta,
        std::min(max_delta, warp_change_request)
    );

    return warp_change_request;
}
