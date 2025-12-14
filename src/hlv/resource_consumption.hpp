#pragma once

#include <algorithm>
#include <cmath>

// Resource Consumption Dynamics
inline void consume_resources(
    SpacetimeModulationState& state,
    const SpacetimeModulationCommand& command,
    uint32_t elapsed_ms) {

    // Antimatter consumption
    float antimatter_consumed =
        state.power_draw_GW *
        ANTIMATTER_BURN_RATE_GW_TO_KG_PER_MS *
        elapsed_ms;

    state.remaining_antimatter_kg =
        std::max(
            0.0f,
            state.remaining_antimatter_kg - antimatter_consumed
        );

    // Quantum fluid consumption (nonlinear with curvature)
    float C = state.spacetime_curvature_magnitude;

    float fluid_consumed_base =
        QUANTUM_FLUID_BASE_CONSUMPTION_RATE *
        elapsed_ms;

    float fluid_consumed_curvature =
        std::pow(
            C,
            QUANTUM_FLUID_CONSUMPTION_CURVATURE_EXPONENT
        ) *
        QUANTUM_FLUID_CONSUMPTION_PER_CURVATURE_UNIT_MS *
        elapsed_ms;

    // Fluid injection
    float fluid_injected =
        command.target_quantum_fluid_flow_rate *
        (elapsed_ms / 1000.0f);

    state.quantum_fluid_level +=
        fluid_injected -
        fluid_consumed_base -
        fluid_consumed_curvature;

    state.quantum_fluid_level = std::max(
        0.0f,
        std::min(
            INITIAL_QUANTUM_FLUID_LITERS * 1.2f,
            state.quantum_fluid_level
        )
    );
}
