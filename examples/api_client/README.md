# HLV-RAPS REST API Examples

This directory contains examples for using the HLV-RAPS REST API.

## Files

- **`rest_api_demo.cpp`**: C++ demo server that shows how to integrate the REST API into your application
- **`api_client.py`**: Python client example for querying the API
- **`CMakeLists.txt`**: Build configuration for the C++ demo

## Building the Demo Server

```bash
cd examples/api_client
cmake -S . -B build
cmake --build build
```

## Running the Demo Server

```bash
./build/rest_api_demo
```

The server will start on `0.0.0.0:8080` and provide mock telemetry data.

## Using the Python Client

```bash
# Install Python 3 (if not already installed)
# No additional dependencies required - uses only standard library

# Query the API
python3 api_client.py localhost 8080
```

## Testing with curl

```bash
# Health check
curl http://localhost:8080/health

# Get current state
curl http://localhost:8080/api/state

# Get PDT predictions
curl http://localhost:8080/api/pdt

# Get DSM safety status
curl http://localhost:8080/api/dsm

# Get supervisor state
curl http://localhost:8080/api/supervisor

# Get rollback status
curl http://localhost:8080/api/rollback

# Get ITL entries
curl http://localhost:8080/api/itl
```

## Integration Guide

To integrate the REST API into your RAPS application:

1. **Include the headers:**
   ```cpp
   #include "raps/api/rest_api_server.hpp"
   ```

2. **Create a snapshot provider:**
   ```cpp
   auto snapshot_provider = []() -> raps::api::SystemSnapshot {
       raps::api::SystemSnapshot snapshot{};
       
       // Fill in snapshot data from your actual system state
       // (PDT, DSM, Supervisor, Rollback, ITL, etc.)
       
       return snapshot;
   };
   ```

3. **Start the API server:**
   ```cpp
   raps::api::RestApiServer api_server;
   api_server.set_snapshot_provider(snapshot_provider);
   
   if (api_server.start(8080, "0.0.0.0")) {
       std::cout << "API server started\n";
   }
   
   // Server runs in background thread
   // ...
   
   // Stop on shutdown
   api_server.stop();
   ```

4. **Important**: The snapshot provider callback runs with mutex protection and should:
   - Be fast (avoid blocking the API requests)
   - Return consistent snapshots (all data from the same moment)
   - Access shared state with proper synchronization

## API Documentation

See [`../../docs/REST_API.md`](../../docs/REST_API.md) for complete API documentation.

## Security Notes

**Important**: This API is designed for observability in a trusted LAN environment:

- ✓ Read-only (no control surfaces)
- ✓ Lightweight (no heavy dependencies)
- ✓ Thread-safe (mutex-protected)
- ✗ No authentication
- ✗ No encryption (HTTP, not HTTPS)

For production flight systems:
1. Deploy on an isolated network segment
2. Use firewall rules to restrict access
3. Consider adding TLS/authentication if needed for your environment
4. Monitor API access via network logs
