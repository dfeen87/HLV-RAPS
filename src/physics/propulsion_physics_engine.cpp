#include "PropulsionPhysicsEngine.hpp"
#include "RAPSDefinitions.hpp"

#include <algorithm>
#include <cmath>

// Predicts the future state based on current state and control inputs
PhysicsState PropulsionPhysicsEngine::predict_state(
    const PhysicsState& initial_state,
    const PhysicsControlInput& control_input) const
{
    PhysicsState next_state = initial_state;
    uint32_t remaining_time_ms = control_input.simulation_duration_ms;

    // Convert thrust from kN to N
    const float thrust_mag_N =
        control_input.thrust_magnitude_kN * 1000.0f;

    const float flow_rate =
        control_input.propellant_flow_kg_s;

    // Thrust direction (simplified spherical coordinates)
    float theta = control_input.gimbal_theta_rad;
    float phi   = control_input.gimbal_phi_rad;

    std::array<float, 3> thrust_dir_vec = {
        std::sin(theta) * std::cos(phi),
        std::sin(theta) * std::sin(phi),
        std::cos(theta)
    };

    thrust_dir_vec = normalize(thrust_dir_vec);

    // Euler integration loop (fixed timestep)
    while (remaining_time_ms > 0) {
        uint32_t dt_ms =
            std::min(remaining_time_ms, PHYSICS_DT_MS);

        float dt_s = static_cast<float>(dt_ms) / 1000.0f;

        // 1. Acceleration
        std::array<float, 3> acc = calculate_acceleration(
            next_state.position_m,
            next_state.velocity_m_s,
            next_state.mass_kg,
            thrust_mag_N,
            thrust_dir_vec
        );

        // 2. Velocity update
        next_state.velocity_m_s[0] += acc[0] * dt_s;
        next_state.velocity_m_s[1] += acc[1] * dt_s;
        next_state.velocity_m_s[2] += acc[2] * dt_s;

        // 3. Position update
        next_state.position_m[0] += next_state.velocity_m_s[0] * dt_s;
        next_state.position_m[1] += next_state.velocity_m_s[1] * dt_s;
        next_state.position_m[2] += next_state.velocity_m_s[2] * dt_s;

        // 4. Mass update
        float mass_loss = flow_rate * dt_s;
        next_state.mass_kg =
            std::max(MIN_MASS_KG, next_state.mass_kg - mass_loss);

        // 5. Time update
        remaining_time_ms -= dt_ms;
        next_state.timestamp_ms += dt_ms;
    }

    return next_state;
}

// Plausibility checks for real-time safety validation
bool PropulsionPhysicsEngine::is_state_physically_plausible(
    const PhysicsState& state) const
{
    if (state.mass_kg < MIN_MASS_KG)
        return false;

    float radius = std::sqrt(
        state.position_m[0]*state.position_m[0] +
        state.position_m[1]*state.position_m[1] +
        state.position_m[2]*state.position_m[2]
    );

    // 10% inside Earth = invalid
    if (radius < RAPSConfig::R_EARTH_M * 0.9f)
        return false;

    // Velocity sanity check
    if (state.velocity_m_s[0] < MIN_VELOCITY_M_S ||
        state.velocity_m_s[1] < MIN_VELOCITY_M_S ||
        state.velocity_m_s[2] < MIN_VELOCITY_M_S)
        return false;

    return true;
}
