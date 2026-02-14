# HLV-RAPS REST API Documentation

## Overview

The HLV-RAPS Flight Middleware provides a **read-only HTTP/JSON REST API** for observability and monitoring. This API is designed for flight-software telemetry access without any control surfaces.

**Key Properties:**
- **Read-only**: Only GET endpoints are supported
- **Non-blocking**: Runs in a dedicated thread to avoid blocking the governance loop
- **Thread-safe**: All shared data access is mutex-protected
- **LAN-accessible**: Binds to `0.0.0.0:8080` by default
- **Lightweight**: Uses POSIX sockets with no heavy dependencies

## Base URL

```
http://<device-ip>:8080
```

Default port is `8080`, configurable at startup.

## Endpoints

### Health Check

**`GET /health`**

Returns basic health and version information.

**Response:**
```json
{
  "status": "ok",
  "service": "HLV-RAPS Flight Middleware",
  "api_version": "1.0",
  "observability_only": true
}
```

### Current State

**`GET /api/state`**

Returns current physical state (Ψ) and informational state (Φ).

**Response:**
```json
{
  "valid": true,
  "timestamp_ms": 123456789,
  "physics_state": {
    "position_m": [100000.0, 50000.0, 25000.0],
    "velocity_m_s": [7000.0, 500.0, 100.0],
    "mass_kg": 50000.0,
    "timestamp_ms": 123456789
  },
  "spacetime_state": {
    "warp_field_strength": 0.85,
    "gravito_flux_bias": 0.12,
    "spacetime_curvature_magnitude": 1.5e-12,
    "time_dilation_factor": 1.00001,
    "induced_gravity_g": 9.81,
    "spacetime_stability_index": 0.95,
    "control_authority_remaining": 0.88,
    "remaining_antimatter_kg": 250.5,
    "emergency_mode_active": false
  }
}
```

**Fields:**
- `valid`: Whether the snapshot is valid
- `timestamp_ms`: Snapshot timestamp
- `physics_state`: Classical physical state (position, velocity, mass)
- `spacetime_state`: HLV-specific informational state (optional, present if available)

### PDT Predictions

**`GET /api/pdt`**

Returns Predictive Digital Twin predictions with confidence and uncertainty.

**Response:**
```json
{
  "valid": true,
  "timestamp_ms": 123456789,
  "status": "NOMINAL",
  "confidence": 0.92,
  "uncertainty": 0.08,
  "prediction_id": "a3f5b2c1d4e6f7a8b9c0d1e2f3a4b5c6d7e8f9a0b1c2d3e4f5a6b7c8d9e0f1a2",
  "predicted_end_state": {
    "position_m": [102000.0, 51000.0, 26000.0],
    "velocity_m_s": [7050.0, 510.0, 105.0],
    "mass_kg": 49950.0,
    "timestamp_ms": 123457089
  }
}
```

**Fields:**
- `status`: Prediction status - `"NOMINAL"`, `"PREDICTED_ESE"`, or `"INVALID"`
- `confidence`: Prediction confidence (0.0 to 1.0)
- `uncertainty`: Prediction uncertainty (0.0 to 1.0)
- `prediction_id`: Unique identifier for this prediction (32-byte hash as hex)
- `predicted_end_state`: Predicted future state

### DSM Safety Status

**`GET /api/dsm`**

Returns Deterministic Safety Monitor status and active safety gates.

**Response:**
```json
{
  "valid": true,
  "timestamp_ms": 123456789,
  "current_action": "NONE",
  "safing_sequence_active": false,
  "last_estimated_curvature": 5.2e-13,
  "measured_time_dilation": 1.00001,
  "measured_oscillatory_prefactor": 0.95,
  "measured_tcc_coupling": 850.0,
  "current_resonance_amplitude": 0.15,
  "main_control_healthy": true
}
```

**Fields:**
- `current_action`: Current safety action - `"NONE"`, `"ROLLBACK"`, or `"FULL_SHUTDOWN"`
- `safing_sequence_active`: Whether a safing sequence is in progress
- `last_estimated_curvature`: Last estimated spacetime curvature scalar (R_max)
- `measured_time_dilation`: Current time dilation measurement
- `measured_oscillatory_prefactor`: Oscillatory modulation stability (A_t)
- `measured_tcc_coupling`: Tri-cell coupling strength (J)
- `current_resonance_amplitude`: Current resonance amplitude
- `main_control_healthy`: Health status of main control system

### Supervisor State

**`GET /api/supervisor`**

Returns redundant supervisor state (A/B channel, failover, mismatch flags).

**Response:**
```json
{
  "valid": true,
  "timestamp_ms": 123456789,
  "active_channel": "A",
  "has_prediction_mismatch": false,
  "last_sync_timestamp_ms": 123456500,
  "last_prediction_confidence": 0.92,
  "last_prediction_uncertainty": 0.08
}
```

**Fields:**
- `active_channel`: Currently active controller - `"A"` or `"B"`
- `has_prediction_mismatch`: Whether predictions between A/B channels diverged
- `last_sync_timestamp_ms`: Timestamp of last synchronization
- `last_prediction_confidence`: Confidence from last prediction
- `last_prediction_uncertainty`: Uncertainty from last prediction

### Rollback Status

**`GET /api/rollback`**

Returns rollback status and last rollback event.

**Response:**
```json
{
  "valid": true,
  "timestamp_ms": 123456789,
  "has_rollback_plan": true,
  "rollback_count": 3,
  "last_rollback_plan": {
    "policy_id": "safe_fallback_001",
    "thrust_magnitude_kN": 150.0,
    "gimbal_theta_rad": 0.05,
    "gimbal_phi_rad": 0.02,
    "rollback_hash": "b4c5d6e7f8a9b0c1d2e3f4a5b6c7d8e9f0a1b2c3d4e5f6a7b8c9d0e1f2a3b4c5"
  }
}
```

**Fields:**
- `has_rollback_plan`: Whether a rollback plan is available
- `rollback_count`: Total number of rollback plans stored
- `last_rollback_plan`: Most recent rollback plan (if available)
  - `policy_id`: Policy identifier
  - `thrust_magnitude_kN`: Thrust magnitude in kN
  - `gimbal_theta_rad`: Gimbal theta angle in radians
  - `gimbal_phi_rad`: Gimbal phi angle in radians
  - `rollback_hash`: Cryptographic hash of rollback plan

### ITL Telemetry

**`GET /api/itl`**

Returns latest N telemetry entries from the Immutable Telemetry Ledger.

**Response:**
```json
{
  "valid": true,
  "timestamp_ms": 123456789,
  "count": 5,
  "entries": [
    {
      "type": 1,
      "timestamp_ms": 123456700,
      "entry_hash": "c6d7e8f9a0b1c2d3e4f5a6b7c8d9e0f1a2b3c4d5e6f7a8b9c0d1e2f3a4b5c6d7",
      "summary": "State snapshot committed"
    },
    {
      "type": 2,
      "timestamp_ms": 123456750,
      "entry_hash": "d8e9f0a1b2c3d4e5f6a7b8c9d0e1f2a3b4c5d6e7f8a9b0c1d2e3f4a5b6c7d8e9",
      "summary": "Prediction commit (confidence: 0.92)"
    }
  ]
}
```

**Fields:**
- `count`: Number of entries returned (up to 32)
- `entries`: Array of telemetry entries
  - `type`: Entry type code (see ITLEntry::Type enum)
  - `timestamp_ms`: Entry timestamp
  - `entry_hash`: Cryptographic hash of entry
  - `summary`: Human-readable summary

## Error Responses

All errors return a JSON object with an `error` field:

```json
{
  "error": "Error message here"
}
```

**HTTP Status Codes:**
- `200 OK`: Request successful
- `404 Not Found`: Endpoint not found
- `405 Method Not Allowed`: Only GET requests are supported
- `500 Internal Server Error`: Server error (e.g., snapshot provider not configured)

## Usage Examples

### curl

```bash
# Health check
curl http://192.168.1.100:8080/health

# Get current state
curl http://192.168.1.100:8080/api/state

# Get PDT predictions
curl http://192.168.1.100:8080/api/pdt

# Get DSM safety status
curl http://192.168.1.100:8080/api/dsm

# Get supervisor state
curl http://192.168.1.100:8080/api/supervisor

# Get rollback status
curl http://192.168.1.100:8080/api/rollback

# Get ITL entries
curl http://192.168.1.100:8080/api/itl
```

### Python

See `examples/api_client.py` for a complete Python client example.

```python
import requests

# Connect to API
base_url = "http://192.168.1.100:8080"

# Health check
health = requests.get(f"{base_url}/health").json()
print(f"Status: {health['status']}")

# Get PDT predictions
pdt = requests.get(f"{base_url}/api/pdt").json()
print(f"Confidence: {pdt['confidence']}")
print(f"Status: {pdt['status']}")
```

### JavaScript

```javascript
// Fetch API example
async function getPdtStatus() {
  const response = await fetch('http://192.168.1.100:8080/api/pdt');
  const data = await response.json();
  console.log('PDT Confidence:', data.confidence);
  console.log('PDT Status:', data.status);
}

getPdtStatus();
```

## Thread Safety

The API server runs in a dedicated thread and uses mutexes to protect all shared data access. This ensures:

1. **No governance loop blocking**: The main control loop is never blocked by API requests
2. **Consistent snapshots**: All data returned in a single request is from the same moment in time
3. **Safe concurrent access**: Multiple clients can query the API simultaneously

## Integration

To integrate the API server into your application:

```cpp
#include "raps/api/rest_api_server.hpp"

// Create snapshot provider callback
auto snapshot_provider = []() -> raps::api::SystemSnapshot {
    raps::api::SystemSnapshot snapshot{};
    
    // Fill in snapshot data from your system state
    // This callback is called with mutex protection
    
    return snapshot;
};

// Create and start server
raps::api::RestApiServer api_server;
api_server.set_snapshot_provider(snapshot_provider);

if (api_server.start(8080, "0.0.0.0")) {
    std::cout << "API server started on port 8080\n";
}

// Server runs in background thread
// ...

// Stop server on shutdown
api_server.stop();
```

## Security Considerations

**Important**: This API is designed for **observability only** in a **trusted LAN environment**.

- **No authentication**: The API does not implement authentication
- **No encryption**: Traffic is not encrypted (HTTP, not HTTPS)
- **Read-only**: No control surfaces are exposed
- **Trusted network**: Intended for use in a secure, isolated flight network

For production flight systems:
1. Deploy on an isolated network segment
2. Use firewall rules to restrict access
3. Consider adding TLS/authentication if needed for your environment
4. Monitor API access via network logs

## Performance

- **Minimal overhead**: Simple HTTP parser, no heavy frameworks
- **Fast response**: Typical response time < 1ms for local queries
- **Bounded memory**: All data structures use fixed-size buffers
- **No allocations**: Designed to avoid dynamic memory allocation in hot paths

## Limitations

- **Single-threaded handling**: Each request is handled sequentially
- **No WebSocket support**: Only request/response pattern
- **Limited request size**: 4KB request buffer
- **Fixed snapshot size**: ITL entries limited to last 32 entries

For high-frequency telemetry streaming, consider using the ITL downlink mechanism instead of polling this API.
