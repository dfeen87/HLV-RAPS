#pragma once

// ============================================================
// HIL Config
// ------------------------------------------------------------
// Goal:
// - Keep HIL settings centralized and build-profile friendly
// - Do NOT mix with SIL fault injection flags
// ============================================================

#ifndef RAPS_ENABLE_HIL
#define RAPS_ENABLE_HIL 1
#endif

// Transport selection (default: TCP loopback “device simulator”)
// You can replace the implementation with Serial/CAN/EtherCAT later.
#ifndef RAPS_HIL_TRANSPORT_TCP
#define RAPS_HIL_TRANSPORT_TCP 1
#endif

// Default endpoints (when TCP is used)
#ifndef RAPS_HIL_TCP_HOST
#define RAPS_HIL_TCP_HOST "127.0.0.1"
#endif

#ifndef RAPS_HIL_TCP_PORT
#define RAPS_HIL_TCP_PORT 49000
#endif

// Timing defaults
#ifndef RAPS_HIL_CYCLE_HZ
#define RAPS_HIL_CYCLE_HZ 50
#endif

#ifndef RAPS_HIL_ACTUATOR_TIMEOUT_MS
#define RAPS_HIL_ACTUATOR_TIMEOUT_MS 120
#endif

// Optional: log HIL IO (noisy; keep off by default)
#ifndef RAPS_HIL_VERBOSE_IO
#define RAPS_HIL_VERBOSE_IO 0
#endif
