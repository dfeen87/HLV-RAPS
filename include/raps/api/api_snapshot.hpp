#pragma once

#include <cstdint>
#include <array>
#include <optional>
#include "raps/core/raps_core_types.hpp"
#include "hlv/spacetime_modulation_types.hpp"

// =====================================================
// REST API Snapshot Data Structures
// =====================================================
// Thread-safe snapshots of system state for observability
// All data is immutable after snapshot creation
// =====================================================

namespace raps::api {

// PDT Snapshot - Predictive Digital Twin state
struct PdtSnapshot {
    PredictionResult::Status status;
    PhysicsState predicted_end_state;
    float confidence;
    float uncertainty;
    uint32_t timestamp_ms;
    uint8_t prediction_id[32];
    bool valid;
};

// DSM Snapshot - Deterministic Safety Monitor state
struct DsmSnapshot {
    enum class SafetyAction {
        NONE = 0,
        ROLLBACK = 1,
        FULL_SHUTDOWN = 2
    };
    
    SafetyAction current_action;
    double last_estimated_curvature;
    bool safing_sequence_active;
    double measured_time_dilation;
    double measured_oscillatory_prefactor;
    double measured_tcc_coupling;
    double current_resonance_amplitude;
    bool main_control_healthy;
    uint32_t timestamp_ms;
    bool valid;
};

// Supervisor Snapshot - Redundant Supervisor state
struct SupervisorSnapshot {
    bool is_channel_a_active;
    bool has_prediction_mismatch;
    uint32_t last_sync_timestamp_ms;
    float last_prediction_confidence;
    float last_prediction_uncertainty;
    uint32_t timestamp_ms;
    bool valid;
};

// Rollback Snapshot - Last rollback event and status
struct RollbackSnapshot {
    bool has_rollback_plan;
    char policy_id[32];
    float thrust_magnitude_kN;
    float gimbal_theta_rad;
    float gimbal_phi_rad;
    uint8_t rollback_hash[32];
    uint32_t rollback_count;
    uint32_t timestamp_ms;
    bool valid;
};

// ITL Entry Snapshot - Simplified telemetry entry
struct ItlEntrySnapshot {
    uint8_t type;  // ITLEntry::Type as uint8_t
    uint32_t timestamp_ms;
    uint8_t entry_hash[32];
    char summary[64];  // Human-readable summary
};

// ITL Snapshot - Latest N telemetry entries
struct ItlSnapshot {
    static constexpr size_t MAX_ENTRIES = 32;
    ItlEntrySnapshot entries[MAX_ENTRIES];
    size_t count;
    uint32_t timestamp_ms;
    bool valid;
};

// State Snapshot - Current physical and informational state
struct StateSnapshot {
    // Physical state (Ψ)
    PhysicsState physics_state;
    
    // Informational/HLV state (Φ) - if available
    SpacetimeModulationState spacetime_state;
    
    uint32_t timestamp_ms;
    bool has_spacetime_state;
    bool valid;
};

// Complete system snapshot
struct SystemSnapshot {
    StateSnapshot state;
    PdtSnapshot pdt;
    DsmSnapshot dsm;
    SupervisorSnapshot supervisor;
    RollbackSnapshot rollback;
    ItlSnapshot itl;
    uint32_t snapshot_timestamp_ms;
};

} // namespace raps::api
