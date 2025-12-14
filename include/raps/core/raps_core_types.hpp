#pragma once

#include <cstdint>
#include <cstring>
#include <array>
#include <optional>

#include "PropulsionPhysicsEngine.hpp"

// =====================================================
// Configuration Constants
// =====================================================

namespace RAPSConfig {

constexpr uint32_t DECISION_HORIZON_MS = 300;
constexpr uint32_t WATCHDOG_MS = 120;

constexpr float MAX_ACCEPTABLE_UNCERTAINTY = 0.25f;
constexpr float MIN_CONFIDENCE_FOR_EXECUTION = 0.85f;

constexpr size_t ITL_QUEUE_SIZE = 128;
constexpr size_t MERKLE_BATCH_SIZE = 32;
constexpr size_t MAX_ROLLBACK_STORE = 16;

constexpr float AILEE_CONFIDENCE_ACCEPTED   = 0.90f;
constexpr float AILEE_CONFIDENCE_BORDERLINE = 0.70f;
constexpr float AILEE_GRACE_THRESHOLD       = 0.72f;

// AILEE Consensus / HLV Targets
static constexpr float NOMINAL_ALTITUDE_TARGET_M     = 100000.0f;
static constexpr float NOMINAL_VELOCITY_TARGET_M_S   = 7000.0f;
static constexpr float ACCEPT_POSITION_DEV_M         = 500.0f;
static constexpr float ACCEPT_VELOCITY_DEV_M_S       = 20.0f;
static constexpr float ACCEPT_MASS_DEV_KG             = 5.0f;

} // namespace RAPSConfig

// =====================================================
// Core Data Structures
// =====================================================

// SHA-256 Hash
struct Hash256 {
    uint8_t data[32];

    bool operator==(const Hash256& other) const {
        return std::memcmp(data, other.data, 32) == 0;
    }

    bool operator!=(const Hash256& other) const {
        return !(*this == other);
    }

    static Hash256 null_hash() {
        Hash256 h{};
        std::memset(h.data, 0, 32);
        return h;
    }

    bool is_null() const {
        return *this == null_hash();
    }
};

// =====================================================
// Prediction Result (from PDT)
// =====================================================

struct PredictionResult {

    enum class Status : uint8_t {
        NOMINAL,
        PREDICTED_ESE,
        INVALID
    };

    Status status;
    PhysicsState predicted_end_state;
    float confidence;
    float uncertainty;
    uint32_t timestamp_ms;
    Hash256 prediction_id;
};

// =====================================================
// Policy (APE Output)
// =====================================================

struct Policy {
    char id[32];
    float thrust_magnitude_kN;
    float gimbal_theta_rad;
    float gimbal_phi_rad;
    float cost;
    Hash256 policy_hash;
};

// =====================================================
// Rollback Plan
// =====================================================

struct RollbackPlan {
    char policy_id[32];
    float thrust_magnitude_kN;
    float gimbal_theta_rad;
    float gimbal_phi_rad;
    Hash256 rollback_hash;
    bool valid;
};
