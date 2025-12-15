#pragma once

#include "raps/hil/hil_device_interface.hpp"
#include "raps/hil/hil_config.hpp"

#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

// ============================================================
// TCP HIL Device (newline-delimited JSON)
// ------------------------------------------------------------
// A lightweight host-side transport for HIL rigs.
// Protocol: request/response JSON lines over TCP.
// - Each request is a single JSON object + '\n'
// - Each response is a single JSON object + '\n'
//
// Expected response schema: {"ok":true, ...} or {"ok":false,"err":"..."}
// ============================================================

class HilTcpDevice final : public HilDeviceInterface {
public:
    HilTcpDevice(std::string host = RAPS_HIL_TCP_HOST, uint16_t port = RAPS_HIL_TCP_PORT);
    ~HilTcpDevice() override;

    bool connect();
    void disconnect();
    bool is_connected() const;

    // Optional: tune socket timeouts (milliseconds)
    void set_io_timeout_ms(uint32_t ms);

    // HilDeviceInterface
    uint32_t now_ms() override;
    Hash256 sha256(const void* data, size_t len) override;
    bool ed25519_sign(const Hash256& msg, uint8_t signature[64]) override;

    bool flash_write(uint32_t address, const void* data, size_t len) override;
    bool flash_read(uint32_t address, void* data, size_t len) override;

    bool actuator_execute(const char* tx_id, float throttle, float valve, uint32_t timeout_ms) override;

    bool downlink_queue(const void* data, size_t len) override;

    void metric_emit(const char* name, float value) override;
    void metric_emit(const char* name, float value, const char* tag_key, const char* tag_value) override;

private:
    // Transport helpers
    bool ensure_connected_locked();
    bool send_line_locked(const std::string& line);
    bool recv_line_locked(std::string& out_line);

    // Simple encoding helpers (no external deps)
    static std::string json_escape(const std::string& s);
    static std::string hex_encode(const uint8_t* data, size_t len);
    static bool hex_decode(const std::string& hex, std::vector<uint8_t>& out);

    // Basic parsing for {"ok":true,...,"hex":"..."} style responses
    static bool parse_ok(const std::string& json, bool& ok);
    static bool extract_string_field(const std::string& json, const char* key, std::string& value);
    static bool extract_u32_field(const std::string& json, const char* key, uint32_t& value);

private:
    std::string host_;
    uint16_t port_;
    uint32_t io_timeout_ms_ = 200; // conservative default

    mutable std::mutex mu_;

    int sock_ = -1;
};
