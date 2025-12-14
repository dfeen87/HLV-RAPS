#pragma once

inline void commit_command_pending(
    ITLManager& itl_manager,
    const std::string& tx_id) {

    ITLEntry entry;
    entry.type = ITLEntry::Type::COMMAND_PENDING;
    entry.timestamp_ms = PlatformHAL::now_ms();
    std::strncpy(
        entry.payload.command_execution.tx_id,
        tx_id.c_str(),
        sizeof(entry.payload.command_execution.tx_id) - 1
    );

    itl_manager.commit(entry);
}

inline void commit_command_commit(
    ITLManager& itl_manager,
    const std::string& tx_id,
    uint64_t timestamp_ms) {

    ITLEntry entry;
    entry.type = ITLEntry::Type::COMMAND_COMMIT;
    entry.timestamp_ms = timestamp_ms;
    std::strncpy(
        entry.payload.command_execution.tx_id,
        tx_id.c_str(),
        sizeof(entry.payload.command_execution.tx_id) - 1
    );

    itl_manager.commit(entry);
}

inline void commit_execution_failure(
    ITLManager& itl_manager,
    const std::string& tx_id,
    uint64_t elapsed_ms) {

    ITLEntry entry;
    entry.type = ITLEntry::Type::EXECUTION_FAILURE;
    entry.timestamp_ms = PlatformHAL::now_ms();
    std::strncpy(
        entry.payload.command_execution.tx_id,
        tx_id.c_str(),
        sizeof(entry.payload.command_execution.tx_id) - 1
    );
    entry.payload.command_execution.elapsed_ms = elapsed_ms;

    itl_manager.commit(entry);
}
