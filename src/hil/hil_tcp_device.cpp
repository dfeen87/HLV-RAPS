#include "raps/hil/hil_tcp_device.hpp"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <iostream>
#include <sstream>

#if defined(_WIN32)
  // Windows sockets not implemented in this repo stub.
  #error "HilTcpDevice currently supports POSIX sockets only."
#else
  #include <arpa/inet.h>
  #include <netdb.h>
  #include <netinet/in.h>
  #include <sys/socket.h>
  #include <sys/types.h>
  #include <unistd.h>
  #include <fcntl.h>
#endif

namespace {

uint32_t monotonic_ms() {
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()
    ).count();
    return static_cast<uint32_t>(ms);
}

} // namespace

HilTcpDevice::HilTcpDevice(std::string host, uint16_t port)
    : host_(std::move(host)), port_(port) {}

HilTcpDevice::~HilTcpDevice() {
    disconnect();
}

void HilTcpDevice::set_io_timeout_ms(uint32_t ms) {
    std::lock_guard<std::mutex> lk(mu_);
    io_timeout_ms_ = ms;
}

bool HilTcpDevice::is_connected() const {
    std::lock_guard<std::mutex> lk(mu_);
    return sock_ >= 0;
}

bool HilTcpDevice::connect() {
    std::lock_guard<std::mutex> lk(mu_);
    if (sock_ >= 0) return true;

    // Resolve host
    struct addrinfo hints {};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    struct addrinfo* res = nullptr;
    std::string port_str = std::to_string(port_);
    if (::getaddrinfo(host_.c_str(), port_str.c_str(), &hints, &res) != 0 || !res) {
        return false;
    }

    int s = ::socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (s < 0) {
        ::freeaddrinfo(res);
        return false;
    }

    // Set recv/send timeouts
    timeval tv{};
    tv.tv_sec = static_cast<int>(io_timeout_ms_ / 1000);
    tv.tv_usec = static_cast<int>((io_timeout_ms_ % 1000) * 1000);

    ::setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    ::setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

    // Connect
    if (::connect(s, res->ai_addr, static_cast<int>(res->ai_addrlen)) != 0) {
        ::close(s);
        ::freeaddrinfo(res);
        return false;
    }

    ::freeaddrinfo(res);
    sock_ = s;
    return true;
}

void HilTcpDevice::disconnect() {
    std::lock_guard<std::mutex> lk(mu_);
    if (sock_ >= 0) {
        ::close(sock_);
        sock_ = -1;
    }
}

bool HilTcpDevice::ensure_connected_locked() {
    if (sock_ >= 0) return true;
    // Attempt connect once (no loops, no delays)
    return connect();
}

bool HilTcpDevice::send_line_locked(const std::string& line) {
    if (!ensure_connected_locked()) return false;

    const char* buf = line.c_str();
    size_t len = line.size();
    size_t sent = 0;

    while (sent < len) {
        ssize_t n = ::send(sock_, buf + sent, len - sent, 0);
        if (n <= 0) return false;
        sent += static_cast<size_t>(n);
    }
    return true;
}

bool HilTcpDevice::recv_line_locked(std::string& out_line) {
    out_line.clear();
    if (!ensure_connected_locked()) return false;

    // Read until '\n'
    char ch = 0;
    while (true) {
        ssize_t n = ::recv(sock_, &ch, 1, 0);
        if (n <= 0) return false;

        if (ch == '\n') break;
        out_line.push_back(ch);

        // Guard: prevent runaway (bad peer)
        if (out_line.size() > 1024 * 1024) return false;
    }
    return true;
}

// ------------------------------------------------------------
// Minimal JSON helpers (no deps)
// ------------------------------------------------------------

std::string HilTcpDevice::json_escape(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 16);
    for (char c : s) {
        switch (c) {
            case '\\': out += "\\\\"; break;
            case '\"': out += "\\\""; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default: out += c; break;
        }
    }
    return out;
}

std::string HilTcpDevice::hex_encode(const uint8_t* data, size_t len) {
    static constexpr char HEX[] = "0123456789abcdef";
    std::string out;
    out.reserve(len * 2);
    for (size_t i = 0; i < len; ++i) {
        uint8_t b = data[i];
        out.push_back(HEX[(b >> 4) & 0xF]);
        out.push_back(HEX[b & 0xF]);
    }
    return out;
}

bool HilTcpDevice::hex_decode(const std::string& hex, std::vector<uint8_t>& out) {
    auto hex_val = [](char c) -> int {
        if ('0' <= c && c <= '9') return c - '0';
        if ('a' <= c && c <= 'f') return 10 + (c - 'a');
        if ('A' <= c && c <= 'F') return 10 + (c - 'A');
        return -1;
    };

    if (hex.size() % 2 != 0) return false;
    out.clear();
    out.reserve(hex.size() / 2);

    for (size_t i = 0; i < hex.size(); i += 2) {
        int hi = hex_val(hex[i]);
        int lo = hex_val(hex[i + 1]);
        if (hi < 0 || lo < 0) return false;
        out.push_back(static_cast<uint8_t>((hi << 4) | lo));
    }
    return true;
}

bool HilTcpDevice::parse_ok(const std::string& json, bool& ok) {
    // very small parser: find `"ok":true` or `"ok":false`
    auto pos = json.find("\"ok\"");
    if (pos == std::string::npos) return false;
    pos = json.find(':', pos);
    if (pos == std::string::npos) return false;

    std::string tail = json.substr(pos + 1);
    // trim leading spaces
    tail.erase(tail.begin(), std::find_if(tail.begin(), tail.end(), [](unsigned char c){ return !std::isspace(c); }));
    if (tail.rfind("true", 0) == 0) { ok = true; return true; }
    if (tail.rfind("false", 0) == 0) { ok = false; return true; }
    return false;
}

bool HilTcpDevice::extract_string_field(const std::string& json, const char* key, std::string& value) {
    std::string k = "\"";
    k += key;
    k += "\"";
    auto pos = json.find(k);
    if (pos == std::string::npos) return false;
    pos = json.find(':', pos);
    if (pos == std::string::npos) return false;
    pos = json.find('\"', pos);
    if (pos == std::string::npos) return false;
    auto end = json.find('\"', pos + 1);
    if (end == std::string::npos) return false;
    value = json.substr(pos + 1, end - (pos + 1));
    return true;
}

bool HilTcpDevice::extract_u32_field(const std::string& json, const char* key, uint32_t& value) {
    std::string k = "\"";
    k += key;
    k += "\"";
    auto pos = json.find(k);
    if (pos == std::string::npos) return false;
    pos = json.find(':', pos);
    if (pos == std::string::npos) return false;

    // scan number
    pos++;
    while (pos < json.size() && std::isspace(static_cast<unsigned char>(json[pos]))) pos++;
    size_t end = pos;
    while (end < json.size() && std::isdigit(static_cast<unsigned char>(json[end]))) end++;
    if (end == pos) return false;

    value = static_cast<uint32_t>(std::stoul(json.substr(pos, end - pos)));
    return true;
}

// ------------------------------------------------------------
// HilDeviceInterface methods
// ------------------------------------------------------------

uint32_t HilTcpDevice::now_ms() {
    // We can either ask the device or use local monotonic.
    // For stability, use local monotonic unless the rig provides authoritative time.
    return monotonic_ms();
}

Hash256 HilTcpDevice::sha256(const void* data, size_t len) {
    Hash256 h{};
    std::memset(h.data, 0, sizeof(h.data));
    if (!data || len == 0) return h;

    // Request remote hash: {"op":"sha256","hex":"..."}
    std::vector<uint8_t> bytes(static_cast<const uint8_t*>(data), static_cast<const uint8_t*>(data) + len);
    std::string payload_hex = hex_encode(bytes.data(), bytes.size());

    std::ostringstream req;
    req << "{\"op\":\"sha256\",\"hex\":\"" << payload_hex << "\"}\n";

    std::lock_guard<std::mutex> lk(mu_);
    std::string resp;
    if (!send_line_locked(req.str()) || !recv_line_locked(resp)) {
        return h;
    }

    bool ok = false;
    if (!parse_ok(resp, ok) || !ok) return h;

    std::string out_hex;
    if (!extract_string_field(resp, "hash", out_hex)) return h;

    std::vector<uint8_t> out;
    if (!hex_decode(out_hex, out) || out.size() < 32) return h;

    std::memcpy(h.data, out.data(), 32);
    return h;
}

bool HilTcpDevice::ed25519_sign(const Hash256& msg, uint8_t signature[64]) {
    if (!signature) return false;

    std::string msg_hex = hex_encode(msg.data, 32);
    std::ostringstream req;
    req << "{\"op\":\"ed25519_sign\",\"msg\":\"" << msg_hex << "\"}\n";

    std::lock_guard<std::mutex> lk(mu_);
    std::string resp;
    if (!send_line_locked(req.str()) || !recv_line_locked(resp)) return false;

    bool ok = false;
    if (!parse_ok(resp, ok) || !ok) return false;

    std::string sig_hex;
    if (!extract_string_field(resp, "sig", sig_hex)) return false;

    std::vector<uint8_t> out;
    if (!hex_decode(sig_hex, out) || out.size() < 64) return false;

    std::memcpy(signature, out.data(), 64);
    return true;
}

bool HilTcpDevice::flash_write(uint32_t address, const void* data, size_t len) {
    if (!data && len > 0) return false;

    std::string hex = "";
    if (data && len > 0) {
        hex = hex_encode(static_cast<const uint8_t*>(data), len);
    }

    std::ostringstream req;
    req << "{\"op\":\"flash_write\",\"addr\":" << address << ",\"hex\":\"" << hex << "\"}\n";

    std::lock_guard<std::mutex> lk(mu_);
    std::string resp;
    if (!send_line_locked(req.str()) || !recv_line_locked(resp)) return false;

    bool ok = false;
    return parse_ok(resp, ok) && ok;
}

bool HilTcpDevice::flash_read(uint32_t address, void* data, size_t len) {
    if (!data && len > 0) return false;

    std::ostringstream req;
    req << "{\"op\":\"flash_read\",\"addr\":" << address << ",\"len\":" << static_cast<uint32_t>(len) << "}\n";

    std::lock_guard<std::mutex> lk(mu_);
    std::string resp;
    if (!send_line_locked(req.str()) || !recv_line_locked(resp)) return false;

    bool ok = false;
    if (!parse_ok(resp, ok) || !ok) return false;

    std::string out_hex;
    if (!extract_string_field(resp, "hex", out_hex)) return false;

    std::vector<uint8_t> out;
    if (!hex_decode(out_hex, out)) return false;

    if (out.size() < len) return false;
    std::memcpy(data, out.data(), len);
    return true;
}

bool HilTcpDevice::actuator_execute(const char* tx_id, float throttle, float valve, uint32_t timeout_ms) {
    if (!tx_id || tx_id[0] == '\0') return false;

    std::ostringstream req;
    req << "{\"op\":\"actuator_execute\","
        << "\"tx_id\":\"" << json_escape(tx_id) << "\","
        << "\"throttle\":" << throttle << ","
        << "\"valve\":" << valve << ","
        << "\"timeout_ms\":" << timeout_ms
        << "}\n";

    std::lock_guard<std::mutex> lk(mu_);
    std::string resp;
    if (!send_line_locked(req.str()) || !recv_line_locked(resp)) return false;

    bool ok = false;
    return parse_ok(resp, ok) && ok;
}

bool HilTcpDevice::downlink_queue(const void* data, size_t len) {
    // In HIL, you can forward downlink to the rig (optional).
    // Here we just push as hex so the rig can capture it.
    if (!data && len > 0) return false;

    std::string hex = "";
    if (data && len > 0) hex = hex_encode(static_cast<const uint8_t*>(data), len);

    std::ostringstream req;
    req << "{\"op\":\"downlink\",\"hex\":\"" << hex << "\"}\n";

    std::lock_guard<std::mutex> lk(mu_);
    std::string resp;
    if (!send_line_locked(req.str()) || !recv_line_locked(resp)) return false;

    bool ok = false;
    return parse_ok(resp, ok) && ok;
}

void HilTcpDevice::metric_emit(const char* name, float value) {
    if (!name) return;

#if RAPS_HIL_VERBOSE_IO
    std::cerr << "[HIL METRIC] " << name << "=" << value << "\n";
#endif

    std::ostringstream req;
    req << "{\"op\":\"metric\",\"name\":\"" << json_escape(name) << "\",\"value\":" << value << "}\n";

    std::lock_guard<std::mutex> lk(mu_);
    std::string resp;
    (void)send_line_locked(req.str());
    (void)recv_line_locked(resp); // ignore errors; metrics are best-effort
}

void HilTcpDevice::metric_emit(const char* name, float value, const char* tag_key, const char* tag_value) {
    if (!name) return;

#if RAPS_HIL_VERBOSE_IO
    std::cerr << "[HIL METRIC] " << name << "=" << value
              << " " << (tag_key ? tag_key : "") << "=" << (tag_value ? tag_value : "") << "\n";
#endif

    std::ostringstream req;
    req << "{\"op\":\"metric_tag\","
        << "\"name\":\"" << json_escape(name) << "\","
        << "\"value\":" << value << ","
        << "\"k\":\"" << json_escape(tag_key ? tag_key : "") << "\","
        << "\"v\":\"" << json_escape(tag_value ? tag_value : "") << "\""
        << "}\n";

    std::lock_guard<std::mutex> lk(mu_);
    std::string resp;
    (void)send_line_locked(req.str());
    (void)recv_line_locked(resp);
}
