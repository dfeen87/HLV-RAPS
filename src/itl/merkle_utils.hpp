#pragma once

inline Hash256 compute_itl_merkle_root(
    const Hash256* entries,
    size_t count) {

    return compute_merkle_root(entries, count);
}

inline void anchor_itl_merkle_root(const Hash256& root) {
    PlatformHAL::metric_emit("itl.merkle_anchor", 1.0f);
    // Underlying anchor implementation
    anchor_merkle_root(root);
}
