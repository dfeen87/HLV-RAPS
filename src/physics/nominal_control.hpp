#pragma once

inline PhysicsControlInput generate_nominal_control_input() {
    PhysicsControlInput input{};
    input.thrust_magnitude_kN =
        PropulsionPhysicsEngine::MAX_THRUST_kN;
    input.gimbal_theta_rad = 0.0f;
    input.gimbal_phi_rad = 0.0f;
    input.propellant_flow_kg_s = 100.0f;
    input.simulation_duration_ms =
        RAPSConfig::DECISION_HORIZON_MS;

    return input;
}
