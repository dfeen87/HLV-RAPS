#include "raps/api/rest_api_server.hpp"

#include <cstring>
#include <sstream>
#include <iomanip>
#include <algorithm>

// POSIX sockets
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>

namespace raps::api {

namespace {

// HTTP response templates
constexpr const char* HTTP_200_HEADER = 
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: application/json\r\n"
    "Access-Control-Allow-Origin: *\r\n"
    "Connection: close\r\n"
    "Content-Length: ";

constexpr const char* HTTP_404_HEADER = 
    "HTTP/1.1 404 Not Found\r\n"
    "Content-Type: application/json\r\n"
    "Connection: close\r\n"
    "Content-Length: ";

constexpr const char* HTTP_500_HEADER = 
    "HTTP/1.1 500 Internal Server Error\r\n"
    "Content-Type: application/json\r\n"
    "Connection: close\r\n"
    "Content-Length: ";

// Parse HTTP request line
bool parse_request_line(const std::string& request, std::string& method, std::string& path) {
    size_t first_space = request.find(' ');
    if (first_space == std::string::npos) return false;
    
    size_t second_space = request.find(' ', first_space + 1);
    if (second_space == std::string::npos) return false;
    
    method = request.substr(0, first_space);
    path = request.substr(first_space + 1, second_space - first_space - 1);
    return true;
}

} // anonymous namespace

RestApiServer::RestApiServer() = default;

RestApiServer::~RestApiServer() {
    stop();
}

bool RestApiServer::start(uint16_t port, const std::string& bind_addr) {
    if (running_.load()) {
        return false;  // Already running
    }

    port_ = port;
    bind_address_ = bind_addr;

    // Create server socket
    server_socket_ = ::socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket_ < 0) {
        return false;
    }

    // Set socket options
    int opt = 1;
    if (::setsockopt(server_socket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        ::close(server_socket_);
        server_socket_ = -1;
        return false;
    }

    // Bind to address
    struct sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port_);
    if (bind_addr == "0.0.0.0") {
        addr.sin_addr.s_addr = INADDR_ANY;
    } else {
        if (::inet_pton(AF_INET, bind_addr.c_str(), &addr.sin_addr) <= 0) {
            ::close(server_socket_);
            server_socket_ = -1;
            return false;
        }
    }

    if (::bind(server_socket_, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        ::close(server_socket_);
        server_socket_ = -1;
        return false;
    }

    // Listen for connections
    if (::listen(server_socket_, 5) < 0) {
        ::close(server_socket_);
        server_socket_ = -1;
        return false;
    }

    // Start server thread
    running_.store(true);
    server_thread_ = std::thread(&RestApiServer::server_thread_main, this);

    return true;
}

void RestApiServer::stop() {
    if (!running_.load()) {
        return;
    }

    running_.store(false);

    // Close server socket to unblock accept()
    if (server_socket_ >= 0) {
        ::shutdown(server_socket_, SHUT_RDWR);
        ::close(server_socket_);
        server_socket_ = -1;
    }

    // Join server thread
    if (server_thread_.joinable()) {
        server_thread_.join();
    }
}

bool RestApiServer::is_running() const {
    return running_.load();
}

void RestApiServer::set_snapshot_provider(SnapshotProvider provider) {
    std::lock_guard<std::mutex> lock(snapshot_mutex_);
    snapshot_provider_ = std::move(provider);
}

void RestApiServer::server_thread_main() {
    while (running_.load()) {
        struct sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);
        
        int client_sock = ::accept(server_socket_, 
                                   (struct sockaddr*)&client_addr, 
                                   &client_len);
        
        if (client_sock < 0) {
            if (running_.load()) {
                // Error accepting connection, but server should continue
                continue;
            } else {
                // Server is shutting down
                break;
            }
        }

        // Set socket timeout
        struct timeval tv{};
        tv.tv_sec = 5;  // 5 second timeout
        tv.tv_usec = 0;
        ::setsockopt(client_sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        ::setsockopt(client_sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

        // Handle client in same thread (simple implementation)
        // For production, consider a thread pool
        handle_client(client_sock);
        
        ::close(client_sock);
    }
}

void RestApiServer::handle_client(int client_sock) {
    // Read request
    char buffer[4096];
    ssize_t bytes_read = ::recv(client_sock, buffer, sizeof(buffer) - 1, 0);
    
    if (bytes_read <= 0) {
        return;
    }
    
    buffer[bytes_read] = '\0';
    std::string request(buffer);
    
    // Handle request and get response
    std::string response = handle_request(request);
    
    // Send response
    ::send(client_sock, response.c_str(), response.length(), 0);
}

std::string RestApiServer::handle_request(const std::string& request) {
    std::string method, path;
    
    if (!parse_request_line(request, method, path)) {
        return json_error(400, "Bad Request");
    }
    
    // Only allow GET requests
    if (method != "GET") {
        return json_error(405, "Method Not Allowed - Only GET requests supported");
    }
    
    return route_request(method, path);
}

std::string RestApiServer::route_request(const std::string& method, const std::string& path) {
    if (path == "/health") {
        return handle_health();
    } else if (path == "/api/state") {
        return handle_state();
    } else if (path == "/api/pdt") {
        return handle_pdt();
    } else if (path == "/api/dsm") {
        return handle_dsm();
    } else if (path == "/api/supervisor") {
        return handle_supervisor();
    } else if (path == "/api/rollback") {
        return handle_rollback();
    } else if (path == "/api/itl") {
        return handle_itl();
    } else {
        return json_error(404, "Endpoint not found");
    }
}

std::string RestApiServer::handle_health() {
    std::ostringstream json;
    json << "{"
         << "\"status\":\"ok\","
         << "\"service\":\"HLV-RAPS Flight Middleware\","
         << "\"api_version\":\"1.0\","
         << "\"observability_only\":true"
         << "}";
    
    return json_response(200, json.str());
}

std::string RestApiServer::handle_state() {
    std::lock_guard<std::mutex> lock(snapshot_mutex_);
    
    if (!snapshot_provider_) {
        return json_error(500, "Snapshot provider not configured");
    }
    
    SystemSnapshot snapshot = snapshot_provider_();
    return json_response(200, state_to_json(snapshot.state));
}

std::string RestApiServer::handle_pdt() {
    std::lock_guard<std::mutex> lock(snapshot_mutex_);
    
    if (!snapshot_provider_) {
        return json_error(500, "Snapshot provider not configured");
    }
    
    SystemSnapshot snapshot = snapshot_provider_();
    return json_response(200, pdt_to_json(snapshot.pdt));
}

std::string RestApiServer::handle_dsm() {
    std::lock_guard<std::mutex> lock(snapshot_mutex_);
    
    if (!snapshot_provider_) {
        return json_error(500, "Snapshot provider not configured");
    }
    
    SystemSnapshot snapshot = snapshot_provider_();
    return json_response(200, dsm_to_json(snapshot.dsm));
}

std::string RestApiServer::handle_supervisor() {
    std::lock_guard<std::mutex> lock(snapshot_mutex_);
    
    if (!snapshot_provider_) {
        return json_error(500, "Snapshot provider not configured");
    }
    
    SystemSnapshot snapshot = snapshot_provider_();
    return json_response(200, supervisor_to_json(snapshot.supervisor));
}

std::string RestApiServer::handle_rollback() {
    std::lock_guard<std::mutex> lock(snapshot_mutex_);
    
    if (!snapshot_provider_) {
        return json_error(500, "Snapshot provider not configured");
    }
    
    SystemSnapshot snapshot = snapshot_provider_();
    return json_response(200, rollback_to_json(snapshot.rollback));
}

std::string RestApiServer::handle_itl() {
    std::lock_guard<std::mutex> lock(snapshot_mutex_);
    
    if (!snapshot_provider_) {
        return json_error(500, "Snapshot provider not configured");
    }
    
    SystemSnapshot snapshot = snapshot_provider_();
    return json_response(200, itl_to_json(snapshot.itl));
}

std::string RestApiServer::json_response(int status_code, const std::string& body) {
    std::ostringstream response;
    
    const char* header = (status_code == 200) ? HTTP_200_HEADER :
                        (status_code == 404) ? HTTP_404_HEADER :
                        HTTP_500_HEADER;
    
    response << header << body.length() << "\r\n\r\n" << body;
    return response.str();
}

std::string RestApiServer::json_error(int status_code, const std::string& message) {
    std::ostringstream json;
    json << "{\"error\":\"" << escape_json_string(message) << "\"}";
    return json_response(status_code, json.str());
}

std::string RestApiServer::state_to_json(const StateSnapshot& snapshot) {
    std::ostringstream json;
    json << std::fixed << std::setprecision(6);
    
    json << "{"
         << "\"valid\":" << (snapshot.valid ? "true" : "false") << ","
         << "\"timestamp_ms\":" << snapshot.timestamp_ms << ","
         << "\"physics_state\":{"
         << "\"position_m\":[" 
         << snapshot.physics_state.position_m[0] << ","
         << snapshot.physics_state.position_m[1] << ","
         << snapshot.physics_state.position_m[2] << "],"
         << "\"velocity_m_s\":["
         << snapshot.physics_state.velocity_m_s[0] << ","
         << snapshot.physics_state.velocity_m_s[1] << ","
         << snapshot.physics_state.velocity_m_s[2] << "],"
         << "\"mass_kg\":" << snapshot.physics_state.mass_kg << ","
         << "\"timestamp_ms\":" << snapshot.physics_state.timestamp_ms
         << "}";
    
    if (snapshot.has_spacetime_state) {
        json << ",\"spacetime_state\":{"
             << "\"warp_field_strength\":" << snapshot.spacetime_state.warp_field_strength << ","
             << "\"gravito_flux_bias\":" << snapshot.spacetime_state.gravito_flux_bias << ","
             << "\"spacetime_curvature_magnitude\":" << snapshot.spacetime_state.spacetime_curvature_magnitude << ","
             << "\"time_dilation_factor\":" << snapshot.spacetime_state.time_dilation_factor << ","
             << "\"induced_gravity_g\":" << snapshot.spacetime_state.induced_gravity_g << ","
             << "\"spacetime_stability_index\":" << snapshot.spacetime_state.spacetime_stability_index << ","
             << "\"control_authority_remaining\":" << snapshot.spacetime_state.control_authority_remaining << ","
             << "\"remaining_antimatter_kg\":" << snapshot.spacetime_state.remaining_antimatter_kg << ","
             << "\"emergency_mode_active\":" << (snapshot.spacetime_state.emergency_mode_active ? "true" : "false")
             << "}";
    }
    
    json << "}";
    return json.str();
}

std::string RestApiServer::pdt_to_json(const PdtSnapshot& snapshot) {
    std::ostringstream json;
    json << std::fixed << std::setprecision(6);
    
    const char* status_str = (snapshot.status == PredictionResult::Status::NOMINAL) ? "NOMINAL" :
                            (snapshot.status == PredictionResult::Status::PREDICTED_ESE) ? "PREDICTED_ESE" :
                            "INVALID";
    
    json << "{"
         << "\"valid\":" << (snapshot.valid ? "true" : "false") << ","
         << "\"timestamp_ms\":" << snapshot.timestamp_ms << ","
         << "\"status\":\"" << status_str << "\","
         << "\"confidence\":" << snapshot.confidence << ","
         << "\"uncertainty\":" << snapshot.uncertainty << ","
         << "\"prediction_id\":\"" << hash_to_hex(snapshot.prediction_id, 32) << "\","
         << "\"predicted_end_state\":{"
         << "\"position_m\":["
         << snapshot.predicted_end_state.position_m[0] << ","
         << snapshot.predicted_end_state.position_m[1] << ","
         << snapshot.predicted_end_state.position_m[2] << "],"
         << "\"velocity_m_s\":["
         << snapshot.predicted_end_state.velocity_m_s[0] << ","
         << snapshot.predicted_end_state.velocity_m_s[1] << ","
         << snapshot.predicted_end_state.velocity_m_s[2] << "],"
         << "\"mass_kg\":" << snapshot.predicted_end_state.mass_kg << ","
         << "\"timestamp_ms\":" << snapshot.predicted_end_state.timestamp_ms
         << "}"
         << "}";
    
    return json.str();
}

std::string RestApiServer::dsm_to_json(const DsmSnapshot& snapshot) {
    std::ostringstream json;
    json << std::fixed << std::setprecision(9);
    
    const char* action_str = (snapshot.current_action == DsmSnapshot::SafetyAction::NONE) ? "NONE" :
                            (snapshot.current_action == DsmSnapshot::SafetyAction::ROLLBACK) ? "ROLLBACK" :
                            "FULL_SHUTDOWN";
    
    json << "{"
         << "\"valid\":" << (snapshot.valid ? "true" : "false") << ","
         << "\"timestamp_ms\":" << snapshot.timestamp_ms << ","
         << "\"current_action\":\"" << action_str << "\","
         << "\"safing_sequence_active\":" << (snapshot.safing_sequence_active ? "true" : "false") << ","
         << "\"last_estimated_curvature\":" << snapshot.last_estimated_curvature << ","
         << "\"measured_time_dilation\":" << snapshot.measured_time_dilation << ","
         << "\"measured_oscillatory_prefactor\":" << snapshot.measured_oscillatory_prefactor << ","
         << "\"measured_tcc_coupling\":" << snapshot.measured_tcc_coupling << ","
         << "\"current_resonance_amplitude\":" << snapshot.current_resonance_amplitude << ","
         << "\"main_control_healthy\":" << (snapshot.main_control_healthy ? "true" : "false")
         << "}";
    
    return json.str();
}

std::string RestApiServer::supervisor_to_json(const SupervisorSnapshot& snapshot) {
    std::ostringstream json;
    json << std::fixed << std::setprecision(6);
    
    json << "{"
         << "\"valid\":" << (snapshot.valid ? "true" : "false") << ","
         << "\"timestamp_ms\":" << snapshot.timestamp_ms << ","
         << "\"active_channel\":\"" << (snapshot.is_channel_a_active ? "A" : "B") << "\","
         << "\"has_prediction_mismatch\":" << (snapshot.has_prediction_mismatch ? "true" : "false") << ","
         << "\"last_sync_timestamp_ms\":" << snapshot.last_sync_timestamp_ms << ","
         << "\"last_prediction_confidence\":" << snapshot.last_prediction_confidence << ","
         << "\"last_prediction_uncertainty\":" << snapshot.last_prediction_uncertainty
         << "}";
    
    return json.str();
}

std::string RestApiServer::rollback_to_json(const RollbackSnapshot& snapshot) {
    std::ostringstream json;
    json << std::fixed << std::setprecision(6);
    
    json << "{"
         << "\"valid\":" << (snapshot.valid ? "true" : "false") << ","
         << "\"timestamp_ms\":" << snapshot.timestamp_ms << ","
         << "\"has_rollback_plan\":" << (snapshot.has_rollback_plan ? "true" : "false") << ","
         << "\"rollback_count\":" << snapshot.rollback_count;
    
    if (snapshot.has_rollback_plan) {
        json << ",\"last_rollback_plan\":{"
             << "\"policy_id\":\"" << escape_json_string(std::string(snapshot.policy_id)) << "\","
             << "\"thrust_magnitude_kN\":" << snapshot.thrust_magnitude_kN << ","
             << "\"gimbal_theta_rad\":" << snapshot.gimbal_theta_rad << ","
             << "\"gimbal_phi_rad\":" << snapshot.gimbal_phi_rad << ","
             << "\"rollback_hash\":\"" << hash_to_hex(snapshot.rollback_hash, 32) << "\""
             << "}";
    }
    
    json << "}";
    return json.str();
}

std::string RestApiServer::itl_to_json(const ItlSnapshot& snapshot) {
    std::ostringstream json;
    
    json << "{"
         << "\"valid\":" << (snapshot.valid ? "true" : "false") << ","
         << "\"timestamp_ms\":" << snapshot.timestamp_ms << ","
         << "\"count\":" << snapshot.count << ","
         << "\"entries\":[";
    
    for (size_t i = 0; i < snapshot.count; ++i) {
        if (i > 0) json << ",";
        
        const auto& entry = snapshot.entries[i];
        json << "{"
             << "\"type\":" << static_cast<int>(entry.type) << ","
             << "\"timestamp_ms\":" << entry.timestamp_ms << ","
             << "\"entry_hash\":\"" << hash_to_hex(entry.entry_hash, 32) << "\","
             << "\"summary\":\"" << escape_json_string(std::string(entry.summary)) << "\""
             << "}";
    }
    
    json << "]}";
    return json.str();
}

std::string RestApiServer::hash_to_hex(const uint8_t* hash, size_t len) {
    std::ostringstream hex;
    hex << std::hex << std::setfill('0');
    
    for (size_t i = 0; i < len; ++i) {
        hex << std::setw(2) << static_cast<int>(hash[i]);
    }
    
    return hex.str();
}

std::string RestApiServer::escape_json_string(const std::string& str) {
    std::ostringstream escaped;
    
    for (char c : str) {
        switch (c) {
            case '"':  escaped << "\\\""; break;
            case '\\': escaped << "\\\\"; break;
            case '\b': escaped << "\\b"; break;
            case '\f': escaped << "\\f"; break;
            case '\n': escaped << "\\n"; break;
            case '\r': escaped << "\\r"; break;
            case '\t': escaped << "\\t"; break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    escaped << "\\u" << std::hex << std::setw(4) 
                           << std::setfill('0') << static_cast<int>(static_cast<unsigned char>(c));
                } else {
                    escaped << c;
                }
        }
    }
    
    return escaped.str();
}

} // namespace raps::api
