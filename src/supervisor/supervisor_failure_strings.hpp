#pragma once

inline const char* supervisor_failure_reason_string(
    RedundantSupervisor::FailureMode mode) {

    switch (mode) {
        case RedundantSupervisor::FailureMode::CRITICAL_ROLLBACK_FAIL:
            return "CRITICAL_ROLLBACK_FAIL";
        case RedundantSupervisor::FailureMode::CRITICAL_NO_ROLLBACK:
            return "CRITICAL_NO_ROLLBACK";
        case RedundantSupervisor::FailureMode::PRIMARY_CHANNEL_LOCKUP:
            return "PRIMARY_CHANNEL_LOCKUP";
        case RedundantSupervisor::FailureMode::MISMATED_PREDICTION:
            return "MISMATED_PREDICTION";
        default:
            return "UNKNOWN";
    }
}
