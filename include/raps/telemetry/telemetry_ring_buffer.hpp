#pragma once
/*
 * Bounded ring buffer for telemetry events.
 * Multi-producer, single-consumer friendly.
 *
 * The design prioritizes:
 * - bounded memory
 * - minimal contention
 * - acceptable ordering (seq numbers)
 *
 * License: MIT (see LICENSE)
 */

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace raps::telemetry {

template <typename T, size_t CapacityPow2>
class TelemetryRingBuffer final {
  static_assert(std::is_trivially_copyable<T>::value,
                "TelemetryRingBuffer requires trivially copyable T");
  static_assert((CapacityPow2 & (CapacityPow2 - 1)) == 0,
                "CapacityPow2 must be a power of two");

public:
  TelemetryRingBuffer() noexcept : _write_idx(0), _read_idx(0), _dropped(0) {}

  // Non-blocking push. If buffer is full, drop event and increment dropped counter.
  bool try_push(const T& item) noexcept {
    const uint64_t w = _write_idx.load(std::memory_order_relaxed);
    const uint64_t r = _read_idx.load(std::memory_order_acquire);

    if ((w - r) >= CapacityPow2) {
      _dropped.fetch_add(1, std::memory_order_relaxed);
      return false;
    }

    _data[w & (CapacityPow2 - 1)] = item;
    _write_idx.store(w + 1, std::memory_order_release);
    return true;
  }

  // Pop one item if available.
  bool try_pop(T& out) noexcept {
    const uint64_t r = _read_idx.load(std::memory_order_relaxed);
    const uint64_t w = _write_idx.load(std::memory_order_acquire);

    if (r == w) return false;

    out = _data[r & (CapacityPow2 - 1)];
    _read_idx.store(r + 1, std::memory_order_release);
    return true;
  }

  uint64_t dropped() const noexcept { return _dropped.load(std::memory_order_relaxed); }
  uint64_t size_approx() const noexcept {
    const uint64_t w = _write_idx.load(std::memory_order_relaxed);
    const uint64_t r = _read_idx.load(std::memory_order_relaxed);
    return (w - r);
  }

private:
  alignas(64) std::atomic<uint64_t> _write_idx;
  alignas(64) std::atomic<uint64_t> _read_idx;
  alignas(64) std::atomic<uint64_t> _dropped;

  // Fixed capacity, no heap.
  T _data[CapacityPow2];
};

} // namespace raps::telemetry
