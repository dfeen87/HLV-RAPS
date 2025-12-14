#pragma once

#include <algorithm>

// Gravito-Flux Control (Section 2)
// Full PID control with rate limiting.
inline float compute_gravito_flux_change(
    const SpacetimeModulationState& state,
    const SpacetimeModulationCommand& command,
    float capability_scale,
    float response_scale,
    float& flux_error_integral,
    float& flux_error_previous,
    uint64_t elapsed_ms) {

    float flux_error =
        command.target_gravito_flux_bias -
        state.gravito_flux_bias;

    float flux_pid_output = compute_pid_output(
        flux_error,
        flux_error_integral,
        flux_error_previous,
        FLUX_KP,
        FLUX_KI,
        FLUX_KD,
        FLUX_INTEGRAL_LIMIT,
        elapsed_ms
    );

    float flux_change_request =
        flux_pid_output * capability_scale * response_scale;

    float max_delta =
        GRAVITO_FLUX_RESPONSE_RATE_PER_MS *
        static_cast<float>(elapsed_ms);

    flux_change_request = std::max(
        -max_delta,
        std::min(max_delta, flux_change_request)
    );

    return flux_change_request;
}
