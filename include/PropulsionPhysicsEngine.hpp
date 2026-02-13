#pragma once

#include <array>
#include <cstdint>

#include "raps/core/raps_core_types.hpp"

class PropulsionPhysicsEngine {
public:
    static constexpr float MAX_THRUST_kN = 2500.0f;
    static constexpr float MIN_MASS_KG = 100.0f;
    static constexpr float MIN_VELOCITY_M_S = -20000.0f;
    static constexpr float MAX_VELOCITY_M_S = 20000.0f;  // ~72,000 km/h
    static constexpr uint32_t PHYSICS_DT_MS = 10;

    void init();

    std::array<float, 3> calculate_acceleration(
        const std::array<float, 3>& pos_m,
        const std::array<float, 3>& vel_m_s,
        float mass_kg,
        float thrust_mag_N,
        const std::array<float, 3>& thrust_dir_vec) const;

    PhysicsState predict_state(
        const PhysicsState& initial_state,
        const PhysicsControlInput& control_input) const;

    bool is_state_physically_plausible(
        const PhysicsState& state) const;
};
