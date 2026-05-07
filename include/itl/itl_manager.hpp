#pragma once

#include <cstddef>
#include <cstdint>

#include "core/raps_definitions.hpp"
#include "platform/platform_hal.hpp"

// Immutable Telemetry Ledger (ITL) Manager
// Owns queueing, durability, flash IO, and Merkle batching lifecycle.
class ITLManager {
private:
    // Fixed-size queue (no heap usage)
    ITLEntry queue_[RAPSConfig::ITL_QUEUE_SIZE];
    size_t queue_head_{0};
    size_t queue_tail_{0};
    size_t queue_count_{0};

    // Merkle batching
    Hash256 merkle_buffer_[RAPSConfig::MERKLE_BATCH_SIZE];
    size_t merkle_count_{0};

    // Persistent storage cursor
    uint32_t flash_write_cursor_{0};

    // Internal helpers (thin wrappers only)
    void process_merkle_batch();

public:
    void init();

    // Non-blocking commit (returns optimistic ID).
    // Returns null_hash() if queue is full.
    Hash256 commit(const ITLEntry& entry);

    // Background processing (low-priority task)
    void flush_pending();

    // Log WNN rollback event
    void log_wnn_rollback_event(double curvature, double prefactor);
};

inline void ITLManager::log_wnn_rollback_event(double curvature, double prefactor) {
    ITLEntry wnn_entry{};
    wnn_entry.type = ITLEntry::Type::WNN_ALERT;
    wnn_entry.timestamp_ms = PlatformHAL::now_ms();
    wnn_entry.payload.wnn_alert.curvature_proxy = curvature;
    wnn_entry.payload.wnn_alert.oscillatory_prefactor = prefactor;
    commit(wnn_entry);

    ITLEntry rollback_entry{};
    rollback_entry.type = ITLEntry::Type::ROLLBACK_COMMIT;
    rollback_entry.timestamp_ms = PlatformHAL::now_ms();
    // Payload for rollback commit (CommandExecutionPayload)
    // we just commit the entry to mark the rollback execution triggered by WNN
    commit(rollback_entry);
}
