#include <iostream>
#include <thread>
#include <chrono>
#include <cstring>

#include "raps/api/rest_api_server.hpp"
#include "platform/platform_hal.hpp"

// Example: Mock data provider for demonstration
// In a real system, this would pull from actual RAPS state
class MockDataProvider {
public:
    MockDataProvider() {
        // Initialize some mock data
        cycle_count_ = 0;
    }

    raps::api::SystemSnapshot get_snapshot() {
        raps::api::SystemSnapshot snapshot{};
        
        // Update cycle count
        cycle_count_++;
        uint32_t now = PlatformHAL::now_ms();
        
        // Mock state data
        snapshot.state.valid = true;
        snapshot.state.timestamp_ms = now;
        snapshot.state.physics_state.position_m[0] = 100000.0f;
        snapshot.state.physics_state.position_m[1] = 50000.0f;
        snapshot.state.physics_state.position_m[2] = 25000.0f;
        snapshot.state.physics_state.velocity_m_s[0] = 7000.0f;
        snapshot.state.physics_state.velocity_m_s[1] = 500.0f;
        snapshot.state.physics_state.velocity_m_s[2] = 100.0f;
        snapshot.state.physics_state.mass_kg = 50000.0f;
        snapshot.state.physics_state.timestamp_ms = now;
        
        snapshot.state.has_spacetime_state = true;
        snapshot.state.spacetime_state.warp_field_strength = 0.85f;
        snapshot.state.spacetime_state.gravito_flux_bias = 0.12f;
        snapshot.state.spacetime_state.spacetime_curvature_magnitude = 1.5e-12f;
        snapshot.state.spacetime_state.time_dilation_factor = 1.00001f;
        snapshot.state.spacetime_state.induced_gravity_g = 9.81f;
        snapshot.state.spacetime_state.spacetime_stability_index = 0.95f;
        snapshot.state.spacetime_state.control_authority_remaining = 0.88f;
        snapshot.state.spacetime_state.remaining_antimatter_kg = 250.5f;
        snapshot.state.spacetime_state.emergency_mode_active = false;
        snapshot.state.spacetime_state.timestamp_ms = now;
        
        // Mock PDT data
        snapshot.pdt.valid = true;
        snapshot.pdt.timestamp_ms = now;
        snapshot.pdt.status = PredictionResult::Status::NOMINAL;
        snapshot.pdt.confidence = 0.92f;
        snapshot.pdt.uncertainty = 0.08f;
        snapshot.pdt.predicted_end_state = snapshot.state.physics_state;
        std::memset(snapshot.pdt.prediction_id, 0xAB, 32);
        
        // Mock DSM data
        snapshot.dsm.valid = true;
        snapshot.dsm.timestamp_ms = now;
        snapshot.dsm.current_action = raps::api::DsmSnapshot::SafetyAction::NONE;
        snapshot.dsm.safing_sequence_active = false;
        snapshot.dsm.last_estimated_curvature = 5.2e-13;
        snapshot.dsm.measured_time_dilation = 1.00001;
        snapshot.dsm.measured_oscillatory_prefactor = 0.95;
        snapshot.dsm.measured_tcc_coupling = 850.0;
        snapshot.dsm.current_resonance_amplitude = 0.15;
        snapshot.dsm.main_control_healthy = true;
        
        // Mock supervisor data
        snapshot.supervisor.valid = true;
        snapshot.supervisor.timestamp_ms = now;
        snapshot.supervisor.is_channel_a_active = true;
        snapshot.supervisor.has_prediction_mismatch = false;
        snapshot.supervisor.last_sync_timestamp_ms = now - 100;
        snapshot.supervisor.last_prediction_confidence = 0.92f;
        snapshot.supervisor.last_prediction_uncertainty = 0.08f;
        
        // Mock rollback data
        snapshot.rollback.valid = true;
        snapshot.rollback.timestamp_ms = now;
        snapshot.rollback.has_rollback_plan = true;
        snapshot.rollback.rollback_count = 3;
        std::strncpy(snapshot.rollback.policy_id, "safe_fallback_001", 
                    sizeof(snapshot.rollback.policy_id) - 1);
        snapshot.rollback.thrust_magnitude_kN = 150.0f;
        snapshot.rollback.gimbal_theta_rad = 0.05f;
        snapshot.rollback.gimbal_phi_rad = 0.02f;
        std::memset(snapshot.rollback.rollback_hash, 0xCD, 32);
        
        // Mock ITL data
        snapshot.itl.valid = true;
        snapshot.itl.timestamp_ms = now;
        snapshot.itl.count = 3;
        
        for (size_t i = 0; i < snapshot.itl.count; ++i) {
            auto& entry = snapshot.itl.entries[i];
            entry.type = static_cast<uint8_t>(i + 1);
            entry.timestamp_ms = now - (2 - i) * 100;
            std::memset(entry.entry_hash, 0xEF + i, 32);
            std::snprintf(entry.summary, sizeof(entry.summary), 
                         "Mock ITL entry %zu", i + 1);
        }
        
        snapshot.snapshot_timestamp_ms = now;
        
        return snapshot;
    }

private:
    uint32_t cycle_count_;
};

int main() {
    std::cout << "==============================================\n";
    std::cout << "  HLV-RAPS REST API Demo\n";
    std::cout << "==============================================\n\n";
    
    // Create mock data provider
    MockDataProvider data_provider;
    
    // Create API server
    raps::api::RestApiServer api_server;
    
    // Set snapshot provider callback
    api_server.set_snapshot_provider([&data_provider]() {
        return data_provider.get_snapshot();
    });
    
    // Start server on port 8080
    std::cout << "Starting API server on 0.0.0.0:8080...\n";
    if (!api_server.start(8080, "0.0.0.0")) {
        std::cerr << "Failed to start API server!\n";
        return 1;
    }
    
    std::cout << "✓ API server started successfully\n\n";
    std::cout << "Available endpoints:\n";
    std::cout << "  http://localhost:8080/health\n";
    std::cout << "  http://localhost:8080/api/state\n";
    std::cout << "  http://localhost:8080/api/pdt\n";
    std::cout << "  http://localhost:8080/api/dsm\n";
    std::cout << "  http://localhost:8080/api/supervisor\n";
    std::cout << "  http://localhost:8080/api/rollback\n";
    std::cout << "  http://localhost:8080/api/itl\n\n";
    
    std::cout << "Try:\n";
    std::cout << "  curl http://localhost:8080/health\n";
    std::cout << "  curl http://localhost:8080/api/pdt\n\n";
    
    std::cout << "Press Ctrl+C to stop server...\n\n";
    
    // Keep running until interrupted
    while (api_server.is_running()) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    std::cout << "\nStopping API server...\n";
    api_server.stop();
    std::cout << "API server stopped.\n";
    
    return 0;
}
