#pragma once

#include <cstddef>
#include <cstdint>

#include "raps/core/raps_definitions.hpp"

// ============================================================
// HIL Device Interface
// ------------------------------------------------------------
// PlatformHAL(HIL) delegates real IO to an implementation of this.
// - In flight: this becomes your true driver layer.
// - In lab: this can be TCP/Serial/CAN shim.
// ============================================================

class HilDeviceInterface {
public:
    virtual ~HilDeviceInterface() = default;

    // Time
    virtual uint32_t now_ms() = 0;

    // Crypto (optional hardware-backed; can be stubbed)
    virtual Hash256 sha256(const void* data, size_t len) = 0;
    virtual bool ed25519_sign(const Hash256& msg, uint8_t signature[64]) = 0;

    // NVM / flash
    virtual bool flash_write(uint32_t address, const void* data, size_t len) = 0;
    virtual bool flash_read(uint32_t address, void* data, size_t len) = 0;

    // Actuation
    virtual bool actuator_execute(
        const char* tx_id,
        float throttle,
        float valve,
        uint32_t timeout_ms
    ) = 0;

    // Downlink / telemetry
    virtual bool downlink_queue(const void* data, size_t len) = 0;

    // Metrics (optional, may be no-op)
    virtual void metric_emit(const char* name, float value) = 0;
    virtual void metric_emit(const char* name, float value, const char* tag_key, const char* tag_value) = 0;
};

// Global injection point used by PlatformHAL(HIL).
// You MUST set this before running the control loop.
void hil_set_device(HilDeviceInterface* dev);
HilDeviceInterface* hil_get_device();
