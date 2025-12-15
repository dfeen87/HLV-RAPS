#pragma once

// ============================================================
// SIL Fault Injection Control
// ------------------------------------------------------------
// Purpose:
// - Deterministic, test-driven fault injection for SIL
// - Explicit control instead of probabilistic-only failures
// - Zero impact unless RAPS_ENABLE_SIL_FAULTS == 1
//
// This file is intentionally header-only.
// ============================================================

#include <atomic>
#include <cstdint>

namespace raps::sil {

// ------------------------------------------------------------
// Fault Types
// ------------------------------------------------------------

enum class FaultType : uint8_t {
    NONE = 0,

    FLASH_WRITE_FAIL,
    FLASH_READ_FAIL,
    ACTUATOR_FAIL,
    ACTUATOR_TIMEOUT,
    DOWNLINK_FAIL,

    METRIC_DROP,
};

// ------------------------------------------------------------
// Fault Control State
// ------------------------------------------------------------

struct FaultState {
    std::atomic<uint32_t> flash_write_fail_count{0};
    std::atomic<uint32_t> flash_read_fail_count{0};
    std::atomic<uint32_t> actuator_fail_count{0};
    std::atomic<uint32_t> actuator_timeout_count{0};
    std::atomic<uint32_t> downlink_fail_count{0};

    std::atomic<bool> metrics_disabled{false};
};

// ------------------------------------------------------------
// Global SIL Fault Controller (singleton-style)
// ------------------------------------------------------------

class FaultInjector {
public:
    static FaultInjector& instance() {
        static FaultInjector inst;
        return inst;
    }

    // ----------- Injection APIs -----------

    void inject_flash_write_fail(uint32_t times = 1) {
        state_.flash_write_fail_count.fetch_add(times);
    }

    void inject_flash_read_fail(uint32_t times = 1) {
        state_.flash_read_fail_count.fetch_add(times);
    }

    void inject_actuator_fail(uint32_t times = 1) {
        state_.actuator_fail_count.fetch_add(times);
    }

    void inject_actuator_timeout(uint32_t times = 1) {
        state_.actuator_timeout_count.fetch_add(times);
    }

    void inject_downlink_fail(uint32_t times = 1) {
        state_.downlink_fail_count.fetch_add(times);
    }

    void disable_metrics(bool disable = true) {
        state_.metrics_disabled.store(disable);
    }

    // ----------- Consumption APIs -----------

    bool consume_flash_write_fail() {
        return consume(state_.flash_write_fail_count);
    }

    bool consume_flash_read_fail() {
        return consume(state_.flash_read_fail_count);
    }

    bool consume_actuator_fail() {
        return consume(state_.actuator_fail_count);
    }

    bool consume_actuator_timeout() {
        return consume(state_.actuator_timeout_count);
    }

    bool consume_downlink_fail() {
        return consume(state_.downlink_fail_count);
    }

    bool metrics_disabled() const {
        return state_.metrics_disabled.load();
    }

    // ----------- Reset (useful for test setup/teardown) -----------

    void reset_all() {
        state_.flash_write_fail_count.store(0);
        state_.flash_read_fail_count.store(0);
        state_.actuator_fail_count.store(0);
        state_.actuator_timeout_count.store(0);
        state_.downlink_fail_count.store(0);
        state_.metrics_disabled.store(false);
    }

private:
    FaultInjector() = default;

    static bool consume(std::atomic<uint32_t>& counter) {
        uint32_t current = counter.load();
        if (current == 0) {
            return false;
        }
        return counter.fetch_sub(1) > 0;
    }

    FaultState state_;
};

} // namespace raps::sil
