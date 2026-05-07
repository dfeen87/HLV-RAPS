#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>

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
    // Returns null_hash() if queue is full or persistence fails.
    Hash256 commit(const ITLEntry& entry);

    // Background processing (low-priority task)
    void flush_pending();

    // Log WNN rollback event. Returns true only when all ledger commits succeed.
    bool log_wnn_rollback_event(double curvature, double prefactor);
};

namespace itl_manager_detail {

inline size_t effective_payload_len(ITLEntry::Type type) {
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
        case ITLEntry::Type::WNN_ALERT:
            return sizeof(ITLEntry::WnnAlertPayload);
        default:
            return 0U;
    }
}

inline Hash256 compute_entry_id(const ITLEntry& entry, size_t payload_len) {
    std::array<uint8_t,
        sizeof(entry.type) + sizeof(entry.timestamp_ms) + sizeof(ITLEntry::PayloadData)> buf{};

    size_t offset = 0U;
    std::memcpy(buf.data() + offset, &entry.type, sizeof(entry.type));
    offset += sizeof(entry.type);
    std::memcpy(buf.data() + offset, &entry.timestamp_ms, sizeof(entry.timestamp_ms));
    offset += sizeof(entry.timestamp_ms);
    if (payload_len > 0U) {
        std::memcpy(buf.data() + offset, &entry.payload, payload_len);
        offset += payload_len;
    }
    return PlatformHAL::sha256(buf.data(), offset);
}

inline Hash256 compute_merkle_root(const Hash256* ids, size_t count) {
    if (ids == nullptr || count == 0U) {
        return Hash256::null_hash();
    }
    if (count == 1U) {
        return ids[0];
    }

    std::array<Hash256, RAPSConfig::MERKLE_BATCH_SIZE> current{};
    std::array<Hash256, RAPSConfig::MERKLE_BATCH_SIZE> next{};
    for (size_t i = 0U; i < count; ++i) {
        current[i] = ids[i];
    }

    size_t current_count = count;
    while (current_count > 1U) {
        size_t next_count = 0U;
        for (size_t i = 0U; i < current_count; i += 2U) {
            const Hash256& left = current[i];
            const Hash256& right = (i + 1U < current_count) ? current[i + 1U] : left;
            std::array<uint8_t, 64> combined{};
            std::memcpy(combined.data(), left.data, 32U);
            std::memcpy(combined.data() + 32U, right.data, 32U);
            next[next_count++] = PlatformHAL::sha256(combined.data(), combined.size());
        }
        for (size_t i = 0U; i < next_count; ++i) {
            current[i] = next[i];
        }
        current_count = next_count;
    }

    return current[0];
}

} // namespace itl_manager_detail

inline void ITLManager::init() {
    queue_head_ = 0U;
    queue_tail_ = 0U;
    queue_count_ = 0U;
    merkle_count_ = 0U;
    flash_write_cursor_ = 0U;
    for (auto& entry : queue_) {
        entry = ITLEntry{};
    }
    for (auto& hash : merkle_buffer_) {
        hash = Hash256::null_hash();
    }
}

inline Hash256 ITLManager::commit(const ITLEntry& entry_template) {
    if (queue_count_ >= RAPSConfig::ITL_QUEUE_SIZE) {
        PlatformHAL::metric_emit("itl.commit_dropped", 1.0f, "reason", "queue_full");
        return Hash256::null_hash();
    }

    ITLEntry entry = entry_template;
    if (entry.timestamp_ms == 0U) {
        entry.timestamp_ms = PlatformHAL::now_ms();
    }

    const size_t payload_len = itl_manager_detail::effective_payload_len(entry.type);
    if (payload_len > sizeof(ITLEntry::PayloadData)) {
        PlatformHAL::metric_emit("itl.commit_dropped", 1.0f, "reason", "payload_len");
        return Hash256::null_hash();
    }
    entry.payload_len = static_cast<uint16_t>(payload_len);
    entry.entry_id = itl_manager_detail::compute_entry_id(entry, payload_len);

    if (entry.entry_id.is_null()) {
        PlatformHAL::metric_emit("itl.commit_dropped", 1.0f, "reason", "entry_hash");
        return Hash256::null_hash();
    }

    if (!PlatformHAL::flash_write(flash_write_cursor_, &entry, sizeof(entry))) {
        PlatformHAL::metric_emit("itl.commit_dropped", 1.0f, "reason", "flash_write");
        return Hash256::null_hash();
    }
    flash_write_cursor_ += static_cast<uint32_t>(sizeof(entry));

    queue_[queue_tail_] = entry;
    queue_tail_ = (queue_tail_ + 1U) % RAPSConfig::ITL_QUEUE_SIZE;
    ++queue_count_;

    merkle_buffer_[merkle_count_++] = entry.entry_id;
    if (merkle_count_ >= RAPSConfig::MERKLE_BATCH_SIZE) {
        process_merkle_batch();
    }

    return entry.entry_id;
}

inline void ITLManager::process_merkle_batch() {
    if (merkle_count_ == 0U) {
        return;
    }

    const Hash256 root = itl_manager_detail::compute_merkle_root(
        merkle_buffer_,
        merkle_count_
    );
    if (!root.is_null()) {
        ITLEntry anchor{};
        anchor.type = ITLEntry::Type::MERKLE_ANCHOR;
        anchor.timestamp_ms = PlatformHAL::now_ms();
        anchor.payload.merkle_anchor.merkle_root = root;
        const size_t payload_len = itl_manager_detail::effective_payload_len(anchor.type);
        anchor.payload_len = static_cast<uint16_t>(payload_len);
        anchor.entry_id = itl_manager_detail::compute_entry_id(anchor, payload_len);
        if (!anchor.entry_id.is_null()) {
            (void)PlatformHAL::flash_write(flash_write_cursor_, &anchor, sizeof(anchor));
            flash_write_cursor_ += static_cast<uint32_t>(sizeof(anchor));
        }
    }

    for (auto& hash : merkle_buffer_) {
        hash = Hash256::null_hash();
    }
    merkle_count_ = 0U;
}

inline void ITLManager::flush_pending() {
    process_merkle_batch();

    while (queue_count_ > 0U) {
        queue_[queue_head_] = ITLEntry{};
        queue_head_ = (queue_head_ + 1U) % RAPSConfig::ITL_QUEUE_SIZE;
        --queue_count_;
    }
}

inline bool ITLManager::log_wnn_rollback_event(double curvature, double prefactor) {
    ITLEntry wnn_entry{};
    wnn_entry.type = ITLEntry::Type::WNN_ALERT;
    wnn_entry.timestamp_ms = PlatformHAL::now_ms();
    wnn_entry.payload.wnn_alert.curvature_proxy = curvature;
    wnn_entry.payload.wnn_alert.oscillatory_prefactor = prefactor;
    const Hash256 wnn_id = commit(wnn_entry);

    ITLEntry rollback_entry{};
    rollback_entry.type = ITLEntry::Type::ROLLBACK_COMMIT;
    rollback_entry.timestamp_ms = PlatformHAL::now_ms();
    const Hash256 rollback_id = commit(rollback_entry);

    return !wnn_id.is_null() && !rollback_id.is_null();
}
