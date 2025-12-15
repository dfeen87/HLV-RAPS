// ============================================================
// HIL Rig Server (C++)
// ------------------------------------------------------------
// - TCP JSON line protocol
// - Deterministic + CI-friendly
// - Mirrors HilTcpDevice expectations exactly
//
// Build (Linux/macOS):
//   g++ -std=c++17 hil_rig_server.cpp -o hil_rig
//
// Run:
//   ./hil_rig            (listens on 127.0.0.1:5555)
// ============================================================

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

// ------------------------------------------------------------
// Config
// ------------------------------------------------------------

static constexpr uint16_t HIL_PORT = 5555;
static constexpr size_t   MAX_LINE = 1024 * 1024;

// ------------------------------------------------------------
// Utilities
// ------------------------------------------------------------

uint32_t now_ms() {
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()
    ).count();
    return static_cast<uint32_t>(ms);
}

std::string hex_encode(const uint8_t* data, size_t len) {
    static constexpr char HEX[] = "0123456789abcdef";
    std::string out;
    out.reserve(len * 2);
    for (size_t i = 0; i < len; ++i) {
        out.push_back(HEX[(data[i] >> 4) & 0xF]);
        out.push_back(HEX[data[i] & 0xF]);
    }
    return out;
}

bool hex_decode(const std::string& hex, std::vector<uint8_t>& out) {
    auto hv = [](char c) -> int {
        if ('0' <= c && c <= '9') return c - '0';
        if ('a' <= c && c <= 'f') return 10 + (c - 'a');
        if ('A' <= c && c <= 'F') return 10 + (c - 'A');
        return -1;
    };

    if (hex.size() % 2 != 0) return false;
    out.clear();

    for (size_t i = 0; i < hex.size(); i += 2) {
        int hi = hv(hex[i]);
        int lo = hv(hex[i + 1]);
        if (hi < 0 || lo < 0) return false;
        out.push_back(static_cast<uint8_t>((hi << 4) | lo));
    }
    return true;
}

// Extremely small JSON helpers (by design)
bool json_has(const std::string& j, const char* k) {
    return j.find(std::string("\"") + k + "\"") != std::string::npos;
}

bool json_get_str(const std::string& j, const char* k, std::string& out) {
    auto p = j.find(std::string("\"") + k + "\"");
    if (p == std::string::npos) return false;
    p = j.find('"', j.find(':', p));
    if (p == std::string::npos) return false;
    auto e = j.find('"', p + 1);
    if (e == std::string::npos) return false;
    out = j.substr(p + 1, e - p - 1);
    return true;
}

// ------------------------------------------------------------
// Rig State
// ------------------------------------------------------------

std::mutex rig_mutex;
std::unordered_map<std::string, bool> applied_tx;
std::vector<uint8_t> flash_memory(64 * 1024); // 64 KB

// ------------------------------------------------------------
// Command Handlers
// ------------------------------------------------------------

std::string ok() {
    return "{\"ok\":true}\n";
}

std::string err(const char* e) {
    return std::string("{\"ok\":false,\"err\":\"") + e + "\"}\n";
}

std::string handle_request(const std::string& req) {
    std::lock_guard<std::mutex> lk(rig_mutex);

    // --- SHA256 (stub, deterministic) ---
    if (json_has(req, "sha256")) {
        std::string hex;
        if (!json_get_str(req, "hex", hex)) return err("missing hex");

        std::vector<uint8_t> data;
        hex_decode(hex, data);

        std::array<uint8_t, 32> h{};
        uint64_t acc = data.size();
        for (uint8_t b : data) acc = (acc * 1315423911u) ^ b;

        std::memcpy(h.data(), &acc, sizeof(acc));
        return "{\"ok\":true,\"hash\":\"" + hex_encode(h.data(), 32) + "\"}\n";
    }

    // --- Ed25519 sign (stub) ---
    if (json_has(req, "ed25519_sign")) {
        std::array<uint8_t, 64> sig{};
        sig.fill(0xAB);
        return "{\"ok\":true,\"sig\":\"" + hex_encode(sig.data(), sig.size()) + "\"}\n";
    }

    // --- Flash write ---
    if (json_has(req, "flash_write")) {
        std::string hex;
        uint32_t addr = 0;
        if (!json_get_str(req, "hex", hex)) return err("hex missing");

        auto p = req.find("\"addr\"");
        if (p != std::string::npos) {
            addr = std::stoul(req.substr(req.find(':', p) + 1));
        }

        std::vector<uint8_t> data;
        hex_decode(hex, data);

        if (addr + data.size() > flash_memory.size()) return err("oob");
        std::copy(data.begin(), data.end(), flash_memory.begin() + addr);
        return ok();
    }

    // --- Flash read ---
    if (json_has(req, "flash_read")) {
        uint32_t addr = 0, len = 0;
        auto p1 = req.find("\"addr\"");
        auto p2 = req.find("\"len\"");
        if (p1 == std::string::npos || p2 == std::string::npos) return err("args");

        addr = std::stoul(req.substr(req.find(':', p1) + 1));
        len  = std::stoul(req.substr(req.find(':', p2) + 1));

        if (addr + len > flash_memory.size()) return err("oob");

        return "{\"ok\":true,\"hex\":\"" +
               hex_encode(&flash_memory[addr], len) + "\"}\n";
    }

    // --- Actuator execute (idempotent) ---
    if (json_has(req, "actuator_execute")) {
        std::string tx;
        if (!json_get_str(req, "tx_id", tx)) return err("tx missing");

        if (applied_tx[tx]) return ok(); // idempotent
        applied_tx[tx] = true;
        return ok();
    }

    // --- Downlink ---
    if (json_has(req, "downlink")) {
        return ok();
    }

    // --- Metric ---
    if (json_has(req, "metric")) {
        return ok();
    }

    return err("unknown op");
}

// ------------------------------------------------------------
// Server Loop
// ------------------------------------------------------------

int main() {
    int s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0) {
        perror("socket");
        return 1;
    }

    int yes = 1;
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(HIL_PORT);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (bind(s, (sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        return 1;
    }

    if (listen(s, 1) < 0) {
        perror("listen");
        return 1;
    }

    std::cout << "[HIL RIG] Listening on 127.0.0.1:" << HIL_PORT << "\n";

    while (true) {
        int c = accept(s, nullptr, nullptr);
        if (c < 0) continue;

        std::cout << "[HIL RIG] Client connected\n";
        std::string line;
        char ch;

        while (recv(c, &ch, 1, 0) > 0) {
            if (ch == '\n') {
                std::string resp = handle_request(line);
                send(c, resp.c_str(), resp.size(), 0);
                line.clear();
            } else {
                line.push_back(ch);
                if (line.size() > MAX_LINE) break;
            }
        }

        close(c);
        std::cout << "[HIL RIG] Client disconnected\n";
    }
}
