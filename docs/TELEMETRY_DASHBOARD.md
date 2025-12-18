# RAPS Telemetry & Dashboard (v2.3)

This document defines the **read-only observability layer** for RAPS.

Telemetry exists to improve:
- auditability
- reproducibility
- operator confidence
- post-run debugging
- performance tuning under real-world conditions

Telemetry is explicitly **non-control**:
> It must never alter execution paths, safety logic, or timing contracts.

---

## Design Contract

Telemetry must:
- be bounded in memory (fixed ring buffer)
- avoid heap allocations in hot paths
- tolerate exporter failure (logging must not crash RAPS)
- preserve deterministic behavior (no feedback into control)

Telemetry must not:
- gate decisions
- modify thresholds
- add blocking I/O in the control loop
- require network access to function

---

## Event Model

Each event includes:
- `seq` monotonic best-effort sequence number
- `t_mono_ns` monotonic timestamp (steady clock)
- optional `t_wall_ns` unix epoch time (correlation only)
- `type`, `subsystem`, `severity`
- numeric `code`
- numeric values `v0,v1,v2`
- optional fixed-size `msg`

---

## Export Format: JSON Lines

Telemetry exports as **JSONL**:
- one JSON object per line
- stream-friendly
- easy to parse in Python, jq, Splunk, etc.

Example line:

```json
{"seq":12,"t_mono_ns":12993000,"t_wall_ns":0,"type":"loop_timing","subsystem":"core","severity":"info","code":0,"v0":1000,"v1":25,"v2":700,"msg":""}
