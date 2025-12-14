#pragma once

inline size_t itl_effective_payload_len(
    ITLEntry::Type type) {

    switch (type) {
        case ITLEntry::Type::STATE_SNAPSHOT:
            return sizeof(ITLEntry::StateSnapshotPayload);

        case ITLEntry::Type::PREDICTION_COMMIT:
            return sizeof(ITLEntry::PredictionCommitPayload);

        case ITLEntry::Type::ESE_ALERT:
            return sizeof(ITLEntry::ESEAlertPayload);

        case ITLEntry::Type::POLICY_PREFLIGHT:
            return sizeof(ITLEntry::PolicyPreflightPayload);

        case ITLEntry::Type::COMMAND_PENDING:
        case ITLEntry::Type::EXECUTION_FAILURE:
        case ITLEntry::Type::COMMAND_COMMIT:
        case ITLEntry::Type::ROLLBACK_COMMIT:
            return sizeof(ITLEntry::CommandExecutionPayload);

        case ITLEntry::Type::ROLLBACK_METADATA:
            return sizeof(ITLEntry::RollbackMetadataPayload);

        case ITLEntry::Type::FALLBACK_TRIGGERED:
            return sizeof(ITLEntry::FallbackTriggeredPayload);

        case ITLEntry::Type::MERKLE_ANCHOR:
            return sizeof(ITLEntry::MerkleAnchorPayload);

        case ITLEntry::Type::GOVERNANCE_BUDGET_VIOLATION:
            return sizeof(ITLEntry::GovernanceBudgetViolationPayload);

        case ITLEntry::Type::NOMINAL_TRACE:
            return sizeof(ITLEntry::NominalTracePayload);

        case ITLEntry::Type::SUPERVISOR_EXCEPTION:
            return sizeof(ITLEntry::SupervisorExceptionPayload);

        case ITLEntry::Type::AILEE_SAFETY_STATUS:
            return sizeof(ITLEntry::AileeSafetyStatusPayload);

        case ITLEntry::Type::AILEE_GRACE_RESULT:
            return sizeof(ITLEntry::AileeGraceResultPayload);

        case ITLEntry::Type::AILEE_CONSENSUS_RESULT:
            return sizeof(ITLEntry::AileeConsensusResultPayload);

        default:
            return 0;
    }
}
