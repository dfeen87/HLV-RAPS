#pragma once

inline void commit_ailee_status(
    ITLManager& itl_manager,
    AileeStatus status,
    float confidence) {

    ITLEntry entry;
    entry.type = ITLEntry::Type::AILEE_SAFETY_STATUS;
    entry.timestamp_ms = PlatformHAL::now_ms();
    entry.payload.ailee_safety_status.status = status;
    entry.payload.ailee_safety_status.confidence_at_decision = confidence;

    itl_manager.commit(entry);
}
