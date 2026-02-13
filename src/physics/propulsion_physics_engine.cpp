#include "PropulsionPhysicsEngine.hpp"

#include <algorithm>
#include <cmath>

namespace {

constexpr float kNormalizeEpsilonSq = 1e-12f;

bool normalize_with_mag(
    const std::array<float, 3>& v,
    std::array<float, 3>& unit_out,
    float& mag_out)
{
    const float mag_sq = v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
    if (mag_sq < kNormalizeEpsilonSq) {
        unit_out = {0.0f, 0.0f, 0.0f};
        mag_out = 0.0f;
        return false;
    }
    mag_out = std::sqrt(mag_sq);
    const float inv_mag = 1.0f / mag_out;
    unit_out = {v[0] * inv_mag, v[1] * inv_mag, v[2] * inv_mag};
    return true;
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

    std::array<float, 3> pos_norm = {0.0f, 0.0f, 0.0f};
    float r = 0.0f;
    normalize_with_mag(pos_m, pos_norm, r);

    // Gravity
    if (r > RAPSConfig::R_EARTH_M * 0.5f) {
        float grav_mag =
            -(RAPSConfig::G_GRAVITATIONAL_CONSTANT *
              RAPSConfig::M_EARTH_KG * mass_kg) / (r * r);

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
        std::array<float, 3> vel_norm = {0.0f, 0.0f, 0.0f};
        float vel_mag = 0.0f;
        normalize_with_mag(vel_m_s, vel_norm, vel_mag);

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

    {
        std::array<float, 3> thrust_norm = {0.0f, 0.0f, 0.0f};
        float thrust_mag = 0.0f;
        normalize_with_mag(thrust_dir_vec, thrust_norm, thrust_mag);
        thrust_dir_vec = thrust_norm;
    }

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

    // 10% inside Earth = invalid
    const float radius_sq =
        state.position_m[0]*state.position_m[0] +
        state.position_m[1]*state.position_m[1] +
        state.position_m[2]*state.position_m[2];
    const float min_radius = RAPSConfig::R_EARTH_M * 0.9f;
    if (radius_sq < min_radius * min_radius)
        return false;

    // Velocity sanity check (magnitude per component)
    if (std::abs(state.velocity_m_s[0]) > MAX_VELOCITY_M_S ||
        std::abs(state.velocity_m_s[1]) > MAX_VELOCITY_M_S ||
        std::abs(state.velocity_m_s[2]) > MAX_VELOCITY_M_S)
        return false;

    return true;
}
