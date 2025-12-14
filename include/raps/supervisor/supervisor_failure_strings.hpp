#pragma once

#include <cstdint>

namespace raps::supervisor {

// Keep this enum in sync with RedundantSupervisor::FailureMode.
// If you already have FailureMode elsewhere, you can remove this enum
// and just overload on your existing type.
enum class FailureMode : uint8_t {
    CRITICAL_ROLLBACK_FAIL,
    CRITICAL_NO_ROLLBACK,
    PRIMARY_CHANNEL_LOCKUP,
    MISMATCHED_PREDICTION
};

// Returns a stable string for metrics + ITL payloads.
// Never returns nullptr.
inline const char* supervisor_failure_reason_string(FailureMode mode) {
    switch (mode) {
        case FailureMode::CRITICAL_ROLLBACK_FAIL: return "CRITICAL_ROLLBACK_FAIL";
        case FailureMode::CRITICAL_NO_ROLLBACK:   return "CRITICAL_NO_ROLLBACK";
        case FailureMode::PRIMARY_CHANNEL_LOCKUP: return "PRIMARY_CHANNEL_LOCKUP";
        case FailureMode::MISMATCHED_PREDICTION:  return "MISMATCHED_PREDICTION";
        default:                                  return "UNKNOWN";
    }
}

} // namespace raps::supervisor
