#include "sil_metric_sink.hpp"

// Pull in your HAL time source
#include "raps/platform/platform_hal.hpp"

namespace SilMetricSink {

static std::mutex g_mu;
static std::array<MetricEvent, MAX_EVENTS> g_buf;
static size_t g_head = 0;   // next write
static size_t g_count = 0;  // total valid
static bool g_enabled = true;

static inline void copy_cstr(char* dst, size_t dst_sz, const char* src) {
    if (!dst || dst_sz == 0) return;
    std::memset(dst, 0, dst_sz);
    if (!src) return;
    std::strncpy(dst, src, dst_sz - 1);
}

void enable(bool on) {
    std::lock_guard<std::mutex> lk(g_mu);
    g_enabled = on;
}

bool enabled() {
    std::lock_guard<std::mutex> lk(g_mu);
    return g_enabled;
}

static void write_event(const char* name,
                        float value,
                        const char* tag_key,
                        const char* tag_value) {
    std::lock_guard<std::mutex> lk(g_mu);
    if (!g_enabled) return;

    MetricEvent& e = g_buf[g_head];
    e.timestamp_ms = PlatformHAL::now_ms();
    copy_cstr(e.name, sizeof(e.name), name);
    e.value = value;
    copy_cstr(e.tag_key, sizeof(e.tag_key), tag_key);
    copy_cstr(e.tag_value, sizeof(e.tag_value), tag_value);

    g_head = (g_head + 1) % MAX_EVENTS;
    if (g_count < MAX_EVENTS) g_count++;
}

void emit(const char* name, float value) {
    write_event(name, value, nullptr, nullptr);
}

void emit(const char* name, float value, const char* tag_key, const char* tag_value) {
    write_event(name, value, tag_key, tag_value);
}

void clear() {
    std::lock_guard<std::mutex> lk(g_mu);
    g_head = 0;
    g_count = 0;
    for (auto& e : g_buf) e = MetricEvent{};
}

size_t size() {
    std::lock_guard<std::mutex> lk(g_mu);
    return g_count;
}

MetricEvent get(size_t idx) {
    std::lock_guard<std::mutex> lk(g_mu);
    if (idx >= g_count) return MetricEvent{};
    // Oldest event index in ring:
    size_t oldest = (g_head + MAX_EVENTS - g_count) % MAX_EVENTS;
    size_t pos = (oldest + idx) % MAX_EVENTS;
    return g_buf[pos];
}

static inline bool sv_eq(std::string_view a, const char* b) {
    if (!b) return a.empty();
    return a == std::string_view(b);
}

size_t count_name(std::string_view name) {
    std::lock_guard<std::mutex> lk(g_mu);
    size_t hits = 0;
    size_t oldest = (g_head + MAX_EVENTS - g_count) % MAX_EVENTS;
    for (size_t i = 0; i < g_count; ++i) {
        const MetricEvent& e = g_buf[(oldest + i) % MAX_EVENTS];
        if (sv_eq(name, e.name)) hits++;
    }
    return hits;
}

size_t count_name_tag(std::string_view name,
                      std::string_view tag_key,
                      std::string_view tag_value) {
    std::lock_guard<std::mutex> lk(g_mu);
    size_t hits = 0;
    size_t oldest = (g_head + MAX_EVENTS - g_count) % MAX_EVENTS;
    for (size_t i = 0; i < g_count; ++i) {
        const MetricEvent& e = g_buf[(oldest + i) % MAX_EVENTS];
        if (!sv_eq(name, e.name)) continue;
        if (!sv_eq(tag_key, e.tag_key)) continue;
        if (!sv_eq(tag_value, e.tag_value)) continue;
        hits++;
    }
    return hits;
}

} // namespace SilMetricSink
