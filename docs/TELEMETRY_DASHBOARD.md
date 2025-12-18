# RAPS Telemetry & Dashboard (v2.3)

This document defines the **read-only observability layer** for RAPS.

Telemetry exists to improve:
- Auditability
- Reproducibility
- Operator confidence
- Post-run debugging
- Performance tuning under real-world conditions

Telemetry is explicitly **non-control**:

> It must never alter execution paths, safety logic, or timing contracts.

---

## Design Contract

### Telemetry Must:
- Be bounded in memory (fixed ring buffer)
- Avoid heap allocations in hot paths
- Tolerate exporter failure (logging must not crash RAPS)
- Preserve deterministic behavior (no feedback into control)

### Telemetry Must Not:
- Gate decisions
- Modify thresholds
- Add blocking I/O in the control loop
- Require network access to function

---

## Event Model

Each event includes:
- `seq` — Monotonic best-effort sequence number
- `t_mono_ns` — Monotonic timestamp (steady clock)
- Optional `t_wall_ns` — Unix epoch time (correlation only)
- `type`, `subsystem`, `severity`
- Numeric `code`
- Numeric values `v0`, `v1`, `v2`
- Optional fixed-size `msg`

---

## Export Format: JSON Lines

Telemetry exports as **JSONL**:
- One JSON object per line
- Stream-friendly
- Easy to parse in Python, jq, Splunk, etc.

### Example Line:
```json
{"seq":12,"t_mono_ns":12993000,"t_wall_ns":0,"type":"loop_timing","subsystem":"core","severity":"info","code":0,"v0":1000,"v1":25,"v2":700,"msg":""}
```

---

## Recommended Wiring

1. Instantiate one `TelemetryLogger<4096>` globally or within your core runtime
2. Emit events from core loop, safety gates, and mode transitions
3. Drain to `JsonlSink` either:
   - In a dedicated exporter thread, or
   - At safe cadence points (e.g., every 50 loops)
4. **Avoid draining every iteration**

---

## Post-Run Dashboard (CLI)

Use `tools/telemetry_report.cpp` to generate a lightweight summary report:

```bash
./telemetry_report raps.telemetry.jsonl
```

This is the default v2.3 "dashboard": simple, deterministic, portable.

Future v2.4+ may add a richer HTML visualization, but only after trust is earned.

---

## Operational Notes

- If the ring buffer fills, events are dropped and `dropped_total` increases
- Dropped events are expected under extreme stress; treat it as a load signal
- Export failure must never terminate RAPS

---

# RAPS v2.3.0 — Telemetry & Observability Layer

This release introduces a **production-hardened, read-only telemetry layer** for RAPS.

Telemetry provides deterministic observability into runtime behavior without altering control logic, safety decisions, or execution pathways.

## What's New

- Bounded telemetry event model (`TelemetryEvent`)
- Fixed-size, allocation-free ring buffer for events
- Logger with monotonic timestamps and stable event schema
- JSON Lines exporter sink for append-only audit trails
- Optional post-run CLI report tool (telemetry "dashboard")

## Why It Matters

This release improves:
- Auditability
- Reproducibility
- Post-run debugging
- Confidence in real-world deployments

## Safety Notes

- Telemetry is strictly **non-control**
- Export failure does not crash RAPS
- Ring overflow results in **dropped events** with counters recorded

## No Behavior Change

This release does not modify the RAPS decision engine, safety gates, or control thresholds.
