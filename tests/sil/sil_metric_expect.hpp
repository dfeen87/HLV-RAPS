#pragma once

#include <string_view>
#include <iostream>
#include "sil_assert.hpp"
#include "sil_metric_sink.hpp"

inline void sil_expect_metric_at_least(std::string_view name, size_t min_count) {
    size_t c = SilMetricSink::count_name(name);
    if (c < min_count) {
        std::cerr << "[SIL] Metric '" << name << "' count=" << c
                  << " expected >= " << min_count << "\n";
        SIL_FAIL("Missing expected metric events");
    }
}

inline void sil_expect_metric_tag_at_least(std::string_view name,
                                          std::string_view tag_key,
                                          std::string_view tag_value,
                                          size_t min_count) {
    size_t c = SilMetricSink::count_name_tag(name, tag_key, tag_value);
    if (c < min_count) {
        std::cerr << "[SIL] Metric '" << name << "' tag(" << tag_key << "=" << tag_value
                  << ") count=" << c << " expected >= " << min_count << "\n";
        SIL_FAIL("Missing expected tagged metric events");
    }
}
