#include "PropulsionPhysicsEngine.hpp"

#include <algorithm>
#include <cmath>

namespace {

std::array<float, 3> normalize(const std::array<float, 3>& v) {
    float mag = std::sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
    if (mag < 1e-6f) {
        return {0.0f, 0.0f, 0.0f};
    }
    return {v[0] / mag, v[1] / mag, v[2] / mag};
}

} // namespace

void PropulsionPhysicsEngine::init() {
    // Stateless predictor – nothing to initialize
}

std::array<float, 3> PropulsionPhysicsEngine::calculate_acceleration(
    const std::array<float, 3>& pos_m,
    const std::array<float, 3>& vel_m_s,
    float mass_kg,
    float thrust_mag_N,
    const std::array<float, 3>& thrust_dir_vec) const
{
    std::array<float, 3> net_force = {0.0f, 0.0f, 0.0f};

    float r = std::sqrt(
        pos_m[0]*pos_m[0] +
        pos_m[1]*pos_m[1] +
        pos_m[2]*pos_m[2]
    );

    // Gravity
    if (r > RAPSConfig::R_EARTH_M * 0.5f) {
        float grav_mag =
            -(RAPSConfig::G_GRAVITATIONAL_CONSTANT *
              RAPSConfig::M_EARTH_KG * mass_kg) / (r * r);

        auto pos_norm = normalize(pos_m);
        net_force[0] += grav_mag * pos_norm[0];
        net_force[1] += grav_mag * pos_norm[1];
        net_force[2] += grav_mag * pos_norm[2];
    }

    // Thrust
    net_force[0] += thrust_mag_N * thrust_dir_vec[0];
    net_force[1] += thrust_mag_N * thrust_dir_vec[1];
    net_force[2] += thrust_mag_N * thrust_dir_vec[2];

    // Atmospheric drag (simplified)
    if (r < RAPSConfig::R_EARTH_M + 100000.0f) {
        auto vel_norm = normalize(vel_m_s);
        float vel_mag = std::sqrt(
            vel_m_s[0]*vel_m_s[0] +
            vel_m_s[1]*vel_m_s[1] +
            vel_m_s[2]*vel_m_s[2]
        );

        float drag_mag =
            -RAPSConfig::ATMOSPHERIC_DRAG_COEFF *
            vel_mag * vel_mag;

        net_force[0] += drag_mag * vel_norm[0];
        net_force[1] += drag_mag * vel_norm[1];
        net_force[2] += drag_mag * vel_norm[2];
    }

    // Acceleration = F / m
    return {
        net_force[0] / mass_kg,
        net_force[1] / mass_kg,
        net_force[2] / mass_kg
    };
}

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
