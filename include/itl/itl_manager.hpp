#pragma once

#include <cstddef>
#include <cstdint>

#include "RAPSDefinitions.hpp"
#include "PlatformHAL.hpp"

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
};
