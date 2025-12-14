#include "RedundantSupervisor.hpp"
#include "PlatformHAL.hpp"
#include "RAPSConfig.hpp"

#include <iostream>
#include <cmath>
#include <ctime>
#include <unistd.h> // POSIX demo only

// ---------------------------------------------------------
// Mock sensor reading function (demo-only stub)
// ---------------------------------------------------------
PhysicsState mock_read_sensors(const PhysicsState& last_state) {
    PhysicsState new_state = last_state;

    new_state.timestamp_ms = PlatformHAL::now_ms();

    // Mock velocity drift
    for (int i = 0; i < 3; ++i) {
        new_state.velocity_m_s[i] += PlatformHAL::random_float(-0.5f, 0.5f);
        new_state.position_m[i] +=
            new_state.velocity_m_s[i] *
            ((new_state.timestamp_ms - last_state.timestamp_ms) / 1000.0f);
    }

    // Mock mass loss
    new_state.mass_kg -= PlatformHAL::random_float(0.0f, 10.0f);

    return new_state;
}

int main() {
    std::cout <<
        "========================================================\n"
        " RAPS Kernel HLV Demonstration (RTOS Concepts)\n"
        "========================================================\n";

    PlatformHAL::seed_rng_for_stubs(
        static_cast<uint32_t>(std::time(nullptr))
    );

    RedundantSupervisor supervisor;
    supervisor.init();

    PhysicsState current_state{};
    current_state.position_m = {RAPSConfig::R_EARTH_M, 0.0f, 0.0f};
    current_state.velocity_m_s = {0.0f, 0.0f, 0.0f};
    current_state.attitude_q = {1.0f, 0.0f, 0.0f, 0.0f};
    current_state.mass_kg = 250000.0f;
    current_state.timestamp_ms = PlatformHAL::now_ms();

    constexpr uint32_t RTOS_CYCLE_MS = 50; // 20 Hz
    int cycle_count = 0;

    while (cycle_count < 20) {
        uint32_t cycle_start = PlatformHAL::now_ms();

        current_state = mock_read_sensors(current_state);
        supervisor.run_cycle(current_state);

        if (cycle_count == 10) {
            std::cout << "\n[MAIN] *** SIMULATING PRIMARY CHANNEL LOCKUP ***\n";
            supervisor.notify_failure(
                RedundantSupervisor::FailureMode::PRIMARY_CHANNEL_LOCKUP
            );
            std::cout << "[MAIN] *** FAILOVER SHOULD HAVE OCCURRED ***\n\n";
        }

        uint32_t elapsed = PlatformHAL::now_ms() - cycle_start;
        if (elapsed < RTOS_CYCLE_MS) {
            usleep((RTOS_CYCLE_MS - elapsed) * 1000);
        }

        float radius_km =
            (std::sqrt(
                current_state.position_m[0] * current_state.position_m[0] +
                current_state.position_m[1] * current_state.position_m[1] +
                current_state.position_m[2] * current_state.position_m[2]
            ) - RAPSConfig::R_EARTH_M) / 1000.0f;

        std::cout << "[MAIN] Cycle " << cycle_count++
                  << " | Radius: " << radius_km
                  << " km | Mass: " << current_state.mass_kg << " kg\n";
    }

    std::cout <<
        "\n========================================================\n"
        "Demo Complete. Check metrics for execution trace.\n";

    return 0;
}
