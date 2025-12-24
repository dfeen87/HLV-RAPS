#pragma once

#include <cstdint>
#include <cstring>
#include <optional>

#include "raps/core/raps_core_types.hpp"

// =====================================================
// AILEE Specific Data Structures
// =====================================================

// AILEE validation statuses
enum class AileeStatus : uint8_t {
    UNDEFINED,
    ACCEPTED,
    BORDERLINE,
    OUTRIGHT_REJECTED,
    GRACE_PASS,
    GRACE_FAIL,
    CONSENSUS_PASS,
    CONSENSUS_FAIL
};

// Data payload for AILEE layers
struct AileeDataPayload {
    PredictionResult pred_result;
    std::optional<Policy> proposed_policy;
    float current_raw_confidence;
};

// =====================================================
// Immutable Telemetry Ledger (ITL) Entry
// =====================================================

struct ITLEntry {

    // --- Payload Definitions ---

    struct StateSnapshotPayload {
        Hash256 snapshot_hash;
        PhysicsState current_state;
    };

    struct PredictionCommitPayload {
        Hash256 prediction_id;
        float confidence;
        float uncertainty;
        Hash256 ref_snapshot_id;
        PhysicsState end_state;
    };

    struct ESEAlertPayload {
        Hash256 prediction_id;
        PhysicsState violating_state;
    };

    struct PolicyPreflightPayload {
        Hash256 policy_hash;
        Hash256 prediction_id;
        float cost;
    };

    struct CommandExecutionPayload {
        Hash256 policy_id;
        char tx_id[24];
        Hash256 command_set_hash;
        Hash256 reference_prediction_id;
        uint32_t elapsed_ms;
    };

    struct RollbackMetadataPayload {
        Hash256 policy_id;
        Hash256 rollback_hash;
    };

    struct FallbackTriggeredPayload {
        char reason[32];
    };

    struct MerkleAnchorPayload {
        Hash256 merkle_root;
    };

    struct GovernanceBudgetViolationPayload {
        uint32_t elapsed_ms;
    };

    struct NominalTracePayload { };

    struct SupervisorExceptionPayload {
        char reason[32];
    };

    struct AileeSafetyStatusPayload {
        AileeStatus status;
        float confidence_at_decision;
    };

    struct AileeGraceResultPayload {
        bool grace_pass;
        float confidence_after_grace;
    };

    struct AileeConsensusResultPayload {
        AileeStatus status;
    };

    // --- Union Payload Container ---

    union PayloadData {
        StateSnapshotPayload state_snapshot;
        PredictionCommitPayload prediction_commit;
        ESEAlertPayload ese_alert;
        PolicyPreflightPayload policy_preflight;
        CommandExecutionPayload command_execution;
        RollbackMetadataPayload rollback_metadata;
        FallbackTriggeredPayload fallback_triggered;
        MerkleAnchorPayload merkle_anchor;
        GovernanceBudgetViolationPayload governance_budget_violation;
        NominalTracePayload nominal_trace;
        SupervisorExceptionPayload supervisor_exception;
        AileeSafetyStatusPayload ailee_safety_status;
        AileeGraceResultPayload ailee_grace_result;
        AileeConsensusResultPayload ailee_consensus_result;
    };

    // --- Entry Type ---

    enum class Type : uint8_t {
        STATE_SNAPSHOT,
        PREDICTION_COMMIT,
        ESE_ALERT,
        POLICY_PREFLIGHT,
        COMMAND_PENDING,
        EXECUTION_FAILURE,
        COMMAND_COMMIT,
        ROLLBACK_METADATA,
        ROLLBACK_COMMIT,
        FALLBACK_TRIGGERED,
        MERKLE_ANCHOR,
        GOVERNANCE_BUDGET_VIOLATION,
        NOMINAL_TRACE,
        SUPERVISOR_EXCEPTION,
        AILEE_SAFETY_STATUS,
        AILEE_GRACE_RESULT,
        AILEE_CONSENSUS_RESULT
    };

    // --- ITL Entry Header ---

    Type type;
    uint32_t timestamp_ms;
    Hash256 entry_id;
    PayloadData payload;
    uint16_t payload_len;

    ITLEntry()
        : type(Type::NOMINAL_TRACE),
          timestamp_ms(0),
          payload_len(0) {
        entry_id = Hash256::null_hash();
        std::memset(&payload, 0, sizeof(PayloadData));
    }
};
