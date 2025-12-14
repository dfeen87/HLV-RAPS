#pragma once

inline PhysicsControlInput policy_to_control_input(
    const Policy& policy,
    uint32_t simulation_duration_ms) {

    PhysicsControlInput input{};
    input.thrust_magnitude_kN = policy.thrust_magnitude_kN;
    input.gimbal_theta_rad = policy.gimbal_theta_rad;
    input.gimbal_phi_rad = policy.gimbal_phi_rad;
    input.propellant_flow_kg_s = 100.0f; // Stub value
    input.simulation_duration_ms = simulation_duration_ms;

    return input;
}
