#pragma once

#include <cstdint>
#include <thread>
#include <atomic>
#include <mutex>
#include <string>
#include <functional>

#include "raps/api/api_snapshot.hpp"

// =====================================================
// REST API Server for HLV-RAPS Observability
// =====================================================
// Read-only HTTP/JSON REST API
// Binds to 0.0.0.0:8080 for LAN-wide access
// Runs in dedicated thread, non-blocking
// All data access is mutex-protected
// =====================================================

namespace raps::api {

// Callback type for retrieving system snapshots
// Called from API server thread with mutex protection
using SnapshotProvider = std::function<SystemSnapshot()>;

class RestApiServer {
public:
    RestApiServer();
    ~RestApiServer();

    // Start the API server in a dedicated thread
    // Returns true if server started successfully
    bool start(uint16_t port = 8080, const std::string& bind_addr = "0.0.0.0");

    // Stop the API server and join the thread
    void stop();

    // Check if server is running
    bool is_running() const;

    // Set the snapshot provider callback
    // This function will be called (with mutex protection) when data is needed
    void set_snapshot_provider(SnapshotProvider provider);

    // Disable copying
    RestApiServer(const RestApiServer&) = delete;
    RestApiServer& operator=(const RestApiServer&) = delete;

private:
    // Server thread entry point
    void server_thread_main();

    // Handle incoming client connection
    void handle_client(int client_sock);

    // HTTP request parsing and routing
    std::string handle_request(const std::string& request);
    std::string route_request(const std::string& method, const std::string& path);

    // Endpoint handlers (return JSON strings)
    std::string handle_health();
    std::string handle_state();
    std::string handle_pdt();
    std::string handle_dsm();
    std::string handle_supervisor();
    std::string handle_rollback();
    std::string handle_itl();

    // JSON formatting helpers
    std::string json_response(int status_code, const std::string& body);
    std::string json_error(int status_code, const std::string& message);
    
    // Convert snapshot to JSON
    std::string state_to_json(const StateSnapshot& snapshot);
    std::string pdt_to_json(const PdtSnapshot& snapshot);
    std::string dsm_to_json(const DsmSnapshot& snapshot);
    std::string supervisor_to_json(const SupervisorSnapshot& snapshot);
    std::string rollback_to_json(const RollbackSnapshot& snapshot);
    std::string itl_to_json(const ItlSnapshot& snapshot);

    // Utility functions
    std::string hash_to_hex(const uint8_t* hash, size_t len);
    std::string escape_json_string(const std::string& str);

    // Server state
    std::atomic<bool> running_{false};
    std::thread server_thread_;
    int server_socket_{-1};
    uint16_t port_{8080};
    std::string bind_address_;

    // Snapshot provider
    mutable std::mutex snapshot_mutex_;
    SnapshotProvider snapshot_provider_;
};

} // namespace raps::api
