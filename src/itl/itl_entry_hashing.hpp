#pragma once

#include <array>
#include <cstring>

inline Hash256 compute_itl_entry_id(
    const ITLEntry& entry,
    size_t effective_payload_len) {

    // NOTE: Buffer is sized for the union maximum, but we hash only the active bytes.
    std::array<uint8_t,
        sizeof(entry.type) +
        sizeof(entry.timestamp_ms) +
        sizeof(ITLEntry::PayloadData)> buf{};

    size_t offset = 0;

    std::memcpy(buf.data() + offset, &entry.type, sizeof(entry.type));
    offset += sizeof(entry.type);

    std::memcpy(buf.data() + offset, &entry.timestamp_ms, sizeof(entry.timestamp_ms));
    offset += sizeof(entry.timestamp_ms);

    if (effective_payload_len > 0) {
        std::memcpy(buf.data() + offset, &entry.payload, effective_payload_len);
        offset += effective_payload_len;
    }

    return PlatformHAL::sha256(buf.data(), offset);
}
