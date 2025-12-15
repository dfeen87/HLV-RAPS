#pragma once

#include <cstdint>
#include <cstring>
#include <mutex>
#include <array>
#include <string_view>

namespace SilMetricSink {

struct MetricEvent {
    uint32_t timestamp_ms = 0;
    char name[64] = {0};
    float value = 0.0f;

    // Optional tags
    char tag_key[32] = {0};
    char tag_value[64] = {0};
};

constexpr size_t MAX_EVENTS = 4096;

// Enable/disable capture (enabled by default under SIL)
void enable(bool on);
bool enabled();

// Record events (called by PlatformHAL hook)
void emit(const char* name, float value);
void emit(const char* name, float value, const char* tag_key, const char* tag_value);

// Utilities
void clear();
size_t size();
MetricEvent get(size_t idx);

// Queries (simple + fast)
size_t count_name(std::string_view name);
size_t count_name_tag(std::string_view name, std::string_view tag_key, std::string_view tag_value);

} // namespace SilMetricSink
