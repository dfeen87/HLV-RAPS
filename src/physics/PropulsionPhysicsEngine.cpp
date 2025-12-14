#include "PropulsionPhysicsEngine.hpp"

#include <cmath>
#include <algorithm>
#include <numeric>

namespace {

// Normalize a 3D vector
std::array<float, 3> normalize(const std::array<float, 3>& v) {
    float mag = std::sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
    if (mag < 1e-6f) return {0.0f, 0.0f, 0.0f};
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
