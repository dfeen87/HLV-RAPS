#pragma once

inline void commit_state_snapshot(
    ITLManager& itl_manager,
    const PhysicsState& current_state) {

    ITLEntry entry;
    entry.type = ITLEntry::Type::STATE_SNAPSHOT;
    entry.timestamp_ms = PlatformHAL::now_ms();
    entry.payload.state_snapshot.current_state = current_state;

    itl_manager.commit(entry);
}
