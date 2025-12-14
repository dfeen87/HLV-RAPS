#pragma once

#include <cmath>

// Detect resonance instability by monitoring variance
// in field coupling stress over a rolling window.
inline bool detect_resonance_instability(
    const SpacetimeModulationState& state,
    float* field_coupling_history,
    uint32_t& history_index) {

    // Update history buffer
    field_coupling_history[history_index] =
        state.field_coupling_stress;

    history_index =
        (history_index + 1) % RESONANCE_SAMPLE_COUNT;

    // Compute mean
    float mean = 0.0f;
    for (uint32_t i = 0; i < RESONANCE_SAMPLE_COUNT; ++i) {
        mean += field_coupling_history[i];
    }
    mean /= static_cast<float>(RESONANCE_SAMPLE_COUNT);

    // Compute variance
    float variance = 0.0f;
    for (uint32_t i = 0; i < RESONANCE_SAMPLE_COUNT; ++i) {
        float diff = field_coupling_history[i] - mean;
        variance += diff * diff;
    }
    variance /= static_cast<float>(RESONANCE_SAMPLE_COUNT);

    // High variance + high coupling = resonance
    bool resonant =
        (variance > (mean * 0.05f)) &&
        (mean > 0.5f);

    if (resonant) {
        PlatformHAL::metric_emit(
            "apcu.resonance_detected",
            1.0f,
            "coupling_stress",
            std::to_string(mean).c_str()
        );
    }

    return resonant;
}
