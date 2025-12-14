#pragma once

// Apply resonance suppression by damping control intent.
inline void apply_resonance_suppression(
    float& warp_change,
    float& flux_change) {

    constexpr float suppression_factor = 0.3f;

    warp_change *= suppression_factor;
    flux_change *= suppression_factor;

    PlatformHAL::metric_emit(
        "apcu.resonance_suppression_active",
        1.0f
    );
}
