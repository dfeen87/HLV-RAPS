#pragma once

#include <algorithm>

// Computes instantaneous power draw and consumes resources.
// Slew rates passed to compute_power_draw() are expressed in units/ms.
inline void update_power_and_resources(
    SpacetimeModulationState& state,
    float warp_change_request,
    float flux_change_request,
    float effective_power_budget,
    uint64_t elapsed_ms) {

    // Power Draw & Resource Consumption (Section 7)

    state.power_draw_GW = compute_power_draw(
        warp_change_request / static_cast<float>(elapsed_ms),
        flux_change_request / static_cast<float>(elapsed_ms)
    );

    // Apply effective power budget (constrained by resource capability)
    state.power_draw_GW =
        std::min(effective_power_budget, state.power_draw_GW);

    state.power_draw_GW =
        std::max(MIN_POWER_DRAW_GW, state.power_draw_GW);

    consume_resources(elapsed_ms);
}
