#pragma once

inline ITLEntry build_merkle_anchor_entry(
    const Hash256& root) {

    ITLEntry entry{};
    entry.type = ITLEntry::Type::MERKLE_ANCHOR;
    entry.timestamp_ms = PlatformHAL::now_ms();
    entry.payload.merkle_anchor.merkle_root = root;

    size_t payload_len =
        sizeof(ITLEntry::MerkleAnchorPayload);

    entry.payload_len =
        static_cast<uint16_t>(payload_len);

    entry.entry_id =
        compute_itl_entry_id(entry, payload_len);

    return entry;
}
