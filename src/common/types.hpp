#pragma once

#include <cstdint>
#include <chrono>

namespace apms::common {

// ------------------------------------------------------------
// Time & Clock Types
// ------------------------------------------------------------
// Centralized aliases to ensure consistent interpretation of time
// across hardware, policy, profiling, and verification layers.
//
// Rationale:
// - steady_clock is monotonic and immune to wall-clock adjustments
// - explicit Duration/TimePoint types improve auditability
// - no runtime behavior; aliases only
// ------------------------------------------------------------

using SteadyClock = std::chrono::steady_clock;

using TimePoint = SteadyClock::time_point;
using Duration  = SteadyClock::duration;

using Nanoseconds = std::chrono::nanoseconds;
using Microseconds = std::chrono::microseconds;
using Milliseconds = std::chrono::milliseconds;

// ------------------------------------------------------------
// Common Scalar Types
// ------------------------------------------------------------
// Explicit-width types reduce ambiguity during review and testing.
// ------------------------------------------------------------

using SampleRateHz = std::uint32_t;
using FrameCount   = std::uint32_t;
using ChannelCount = std::uint16_t;

using SequenceNumber = std::uint64_t;

// ------------------------------------------------------------
// Helper Constants
// ------------------------------------------------------------

constexpr double kSecondsPerMillisecond = 1.0 / 1000.0;
constexpr double kMillisecondsPerSecond = 1000.0;

// ------------------------------------------------------------
// Utility Functions (constexpr, side-effect free)
// ------------------------------------------------------------

constexpr double to_seconds(Duration d) noexcept {
    return std::chrono::duration_cast<std::chrono::duration<double>>(d).count();
}

constexpr Nanoseconds to_nanoseconds(Duration d) noexcept {
    return std::chrono::duration_cast<Nanoseconds>(d);
}

} // namespace apms::common
