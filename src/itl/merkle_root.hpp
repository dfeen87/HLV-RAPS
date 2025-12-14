#pragma once

#include <vector>
#include <array>
#include <cstring>

inline Hash256 compute_merkle_root_hash256(
    const Hash256* ids,
    size_t count) {

    if (count == 0) return Hash256::null_hash();
    if (count == 1) return ids[0];

    std::vector<Hash256> current_level(ids, ids + count);

    while (current_level.size() > 1) {
        std::vector<Hash256> next_level;

        for (size_t i = 0; i < current_level.size(); i += 2) {
            Hash256 left = current_level[i];
            Hash256 right = (i + 1 < current_level.size())
                ? current_level[i + 1]
                : left;

            std::array<uint8_t, 64> combined{};
            std::memcpy(combined.data(), left.data, 32);
            std::memcpy(combined.data() + 32, right.data, 32);

            next_level.push_back(
                PlatformHAL::sha256(combined.data(), 64)
            );
        }

        current_level = std::move(next_level);
    }

    return current_level[0];
}
