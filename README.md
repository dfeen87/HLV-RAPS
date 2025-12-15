# RAPS-HLV Flight Middleware
**Advanced Safety & Predictive Intelligence Layer for Flight-Ready Systems**

Powered by the **Helix–Light–Vortex (HLV)**
**Authors:** Don Michael Feeney Jr. & Marcel Krüger  
**License:** MIT

> **Important:** This repository is a **flight-safety architecture and reference implementation** intended for research, simulation, prototyping, and engineering development.  
> It is **not certified** for operational flight use without a complete certification program (requirements, verification, validation, hardware qualification, and regulatory compliance).

---

## What this is

HLV Flight Middleware is a **deterministic safety + predictive intelligence layer** designed to sit between a flight control computer (or avionics controller) and mission-critical subsystems (propulsion, power, thermal, actuation, guidance support tooling).

It upgrades traditional “monitoring” into a **governed control loop**:

- **Predict** what the system will do next (with uncertainty)
- **Validate** proposed actions against hard safety envelopes
- **Execute** only if safe (or enter fallback/rollback)
- **Audit** everything immutably for traceability

The system is built to support **flight-ready engineering practices**:
determinism, bounded latency, safety gating, redundancy, rollback, and immutable telemetry.

---

## Why this is needed (flight readiness rationale)

Modern aerospace systems fail in the margins:
small deviations accumulate across thermal stress, fatigue, timing drift, sensor noise, and coupled subsystem dynamics.

Traditional flight software often:
- detects problems *after* thresholds are crossed
- lacks predictive gating at the decision layer
- provides logs that are hard to trust or reconstruct
- has limited rollback/failover primitives

HLV Flight Middleware is meant to close that gap by introducing:

- **Predictive Digital Twin** (forecast + confidence/uncertainty)
- **Deterministic Safety Monitor** (hard limits, independent checks)
- **Governance loop** (sense → predict → validate → act → audit)
- **Rollback + redundancy** (A/B supervisory + failover hooks)
- **Immutable Telemetry Ledger (ITL)** (Merkle anchoring, auditable traces)

---

## Core concept (dual-state modeling)

The middleware models the system in two coupled layers:

- **Physical state (Ψ):** classical metrics (voltage/current/temp/cycles/stress/position/velocity/etc.)
- **Informational state (Φ):** entropy, degradation history, anomaly geometry, drift and coherence

Coupling is expressed through an effective metric:

\[
g_{\mu\nu}^{eff} = g_{\mu\nu} + \lambda (\partial_\mu \Phi)(\partial_\nu \Phi)
\]

Practically: Φ acts as a *structured memory + distortion field* that influences how the middleware interprets “normal” evolution of Ψ, enabling earlier detection and better predictive safety gating.

---

## Architecture overview

### 1) Predictive Digital Twin (PDT)
The PDT forecasts the near-future state under candidate commands/policies and produces:
- predicted end state
- confidence & uncertainty
- predicted safety excursions (ESE) signals

Key implementations in this repo include:
- deterministic step simulation
- Monte Carlo sampling (uncertainty estimation)
- optional online residual learning hooks

### 2) Deterministic Safety Monitor (DSM)
An independent safety layer enforcing **inviolable bounds**.
The DSM can demand:
- rollback (safe abort)
- full shutdown (catastrophic prevention)
- safe-state restore

DSM is intentionally simple, conservative, and suitable for separation on independent compute/sensor channels.

### 3) RAPS Governance Loop (Zero-trust execution)
A deterministic orchestrator that runs:

1. **SENSE & AUDIT**: snapshot + commit to ITL  
2. **PREDICT & PLAN**: generate candidate policies  
3. **VALIDATE**: AILEE safety gates + DSM checks  
4. **EXECUTE**: idempotent actuator transaction  
5. **ROLLBACK**: if execution fails or integrity breaks  
6. **AUDIT**: immutable telemetry + Merkle anchoring

### 4) Immutable Telemetry Ledger (ITL)
A compact embedded ledger that supports:
- event commits
- Merkle batching
- anchoring roots (for post-flight audit integrity)
- downlink mirroring (optional)

---

## “Flight-ready” engineering properties (what this repo targets)

This middleware is structured to support flight readiness workstreams:

- **Deterministic runtime behavior**
  - fixed-cycle governance loop
  - bounded execution budgets (WATCHDOG_MS)
- **Fail-safe behavior**
  - rollback plans, fallback safe-state, emergency actions
- **Redundancy readiness**
  - supervisor-level A/B control hooks
  - prediction mismatch detection scaffolding
- **Auditability**
  - immutable event ledger
  - Merkle anchoring for tamper-evidence
- **Portability**
  - platform HAL abstraction
  - RTOS mutex abstraction points
- **Testability**
  - demo harness + reference implementations
  - clear seams for simulation and HIL

> Certification still requires: requirements traceability, exhaustive testing, timing analysis, platform qualification, tool qualification, independent V&V, and regulatory compliance planning.

---

## Repository map (high-level)

### Examples
- `examples/hlv_demo/hlv_rtos_demo.cpp`

### Public headers (`include/`)
- `include/apcu/advanced_propulsion_control_unit.hpp`
- `include/config/raps_safety_limits.hpp`
- `include/core/raps_definitions.hpp`
- `include/hlv/hlv_constants.hpp`
- `include/hlv/spacetime_modulation_types.hpp`
- `include/itl/itl_manager.hpp`
- `include/platform/platform_hal.hpp`
- `include/raps/core/raps_core_types.hpp`
- `include/raps/hlv/hlv_field_dynamics.hpp`
- `include/raps/pdt/hlv_pdt_engine.hpp`
- `include/raps/platform/rtos_mutex.hpp`
- `include/raps/safety/deterministic_safety_monitor.hpp`
- `include/raps/safety/safety_monitor.hpp`
- `include/raps/supervisor/redundant_supervisor.hpp`
- `include/raps/supervisor/supervisor_failure_strings.hpp`

### Reference material
- `reference/python/hlv_reference_integrator.hpp`

### Source (`src/`)
- Control
  - `src/control/pid_controller.hpp`
- HLV subsystem modules
  - `src/hlv/artificial_gravity_control.hpp`
  - `src/hlv/capability_scaling.hpp`
  - `src/hlv/derived_gravity_model.hpp`
  - `src/hlv/derived_time_dilation_model.hpp`
  - `src/hlv/efficiency_and_displacement.hpp`
  - `src/hlv/field_coupling_stress_model.hpp`
  - `src/hlv/gravito_flux_control.hpp`
  - `src/hlv/power_and_resource_management.hpp`
  - `src/hlv/power_draw_model.hpp`
  - `src/hlv/resonance_detection.hpp`
  - `src/hlv/resonance_suppression.hpp`
  - `src/hlv/resource_consumption.hpp`
  - `src/hlv/spacetime_curvature_dynamics.hpp`
  - `src/hlv/spacetime_curvature_model.hpp`
  - `src/hlv/subspace_efficiency.hpp`
  - `src/hlv/subspace_efficiency_model.hpp`
  - `src/hlv/time_dilation_control.hpp`
  - `src/hlv/warp_field_control.hpp`
- ITL primitives
  - `src/itl/itl_ailee_status.hpp`
  - `src/itl/itl_command_events.hpp`
  - `src/itl/itl_entry_hashing.hpp`
  - `src/itl/itl_merkle_anchor_entry.hpp`
  - `src/itl/itl_payload_sizing.hpp`
  - `src/itl/itl_state_snapshot.hpp`
  - `src/itl/merkle_root.hpp`
  - `src/itl/merkle_utils.hpp`
- Physics
  - `src/physics/propulsion_physics_engine.cpp`
  - `src/physics/PropulsionPhysicsEngine.cpp` *(if both exist, keep one canonical and deprecate the other)*
  - `src/physics/nominal_control.hpp`
  - `src/physics/policy_to_control_input.hpp`
- RAPS governance & safety
  - `src/raps/rollback_execution.hpp`
  - `src/raps/rollback_store.hpp`
  - `src/raps/state_hashing.hpp`
  - `src/raps/stability_and_authority_metrics.hpp`
  - `src/raps/apcu_state_management_and_safety.hpp`
  - `src/raps/safety/ailee_confidence_classification.hpp`
- Supervisor
  - `src/supervisor/prediction_mismatch_policy.hpp`
  - `src/supervisor/supervisor_failure_strings.hpp`

---

## Quickstart (demo build/run)

This repo is designed to be build-system agnostic (CMake-friendly) and RTOS-portable via `PlatformHAL` + `rtos_mutex` seams.

Typical flow:
1. Build your target (host demo or embedded toolchain)
2. Run the RTOS demo harness:
   - `examples/hlv_demo/hlv_rtos_demo.cpp`

> If you want, add a `docs/BUILD.md` with exact commands for your CMake layout and CI runners.

---

## Operational flow (how it behaves)

1. **Snapshot** current state (Ψ) and derived informational health state (Φ)
2. **Predict** forward (PDT) → produce (state, confidence, uncertainty)
3. If risk is detected:
   - generate candidate policies (APE)
   - validate via AILEE + DSM
4. **Execute** via idempotent actuator transaction
5. If execution fails or deviates:
   - **rollback** immediately using stored rollback plan
6. **Commit** every step to ITL and anchor Merkle batches

---
## Software-in-the-Loop (SIL) & Hardware-in-the-Loop (HIL)

RAPS is designed to be **flight-ready by construction**, not by late-stage testing.  
To support this, the repository now includes **first-class SIL and HIL infrastructure** that allows the entire governance, safety, and prediction stack to be exercised *before* deployment on real hardware.

---

### Software-in-the-Loop (SIL)

The SIL layer provides a **deterministic, CI-friendly execution environment** that runs the full RAPS control, prediction, and safety pipeline on a host machine.

**Key capabilities**
- **PlatformHAL SIL backend**  
  Target-agnostic stubs for time, flash, actuation, telemetry, and metrics.
- **Compile-time fault injection**  
  Controlled via build flags to simulate actuator failures, flash errors, latency spikes, and downlink loss.
- **Deterministic execution mode**  
  Enables reproducible test runs for CI, regression testing, and safety validation.
- **Coverage gates**  
  Explicit pass/fail gates ensure that:
  - rollback paths are exercised,
  - supervisor failover is triggered,
  - safety monitors observe and respond to injected faults.

**Why it matters**
- Validates *governance logic*, not just math.
- Catches race conditions and rollback bugs early.
- Makes safety behavior auditable and repeatable.

---

### Hardware-in-the-Loop (HIL)

The HIL layer bridges RAPS to **real hardware or a physical test rig**, validating that software assumptions hold under real timing, transport, and actuator constraints.

**Key capabilities**
- **HIL-backed PlatformHAL**  
  The same interface used in SIL, but linked against a hardware or rig-server implementation.
- **Rig-driven actuator validation**  
  Confirms idempotent command execution, timeout handling, and transaction safety.
- **Downlink and telemetry verification**  
  Ensures ground-facing observability paths are alive and correctly routed.
- **One-shot readiness executable**  
  `examples/hil/hil_readiness_check.cpp` provides a fast, binary “go / no-go” signal for:
  - timing,
  - hashing,
  - flash access,
  - actuation,
  - downlink.

**Why it matters**
- Proves the software can survive *real* latencies and failures.
- Prevents “it worked in simulation” surprises.
- Provides a clean handoff from lab validation to flight integration.

---

### Unified Design Philosophy

SIL and HIL are not separate code paths — they are **link-time personalities** of the same system.

- The **same governance logic** runs in SIL, HIL, and flight.
- Safety and rollback behavior is validated *before* certification.
- Flight builds simply replace the PlatformHAL backend with certified hardware drivers.

This approach enables RAPS to move from research → simulation → hardware → flight **without rewriting core logic**, preserving correctness, traceability, and safety guarantees at every stage.

---

## Collaboration

This project is MIT licensed and welcomes:
- audits and reviews
- safety envelope expansions
- portability improvements (RTOS/HAL)
- test harnesses (SIL/HIL)
- documentation, diagrams, and spec alignment

---

## Publications / References

RAPS Foundational Document + Part II (HLV Physics Math Implementation):

https://dfeen.substack.com/p/ai-augmented-rigor-a-zero-trust-governance

Preprint (Gaussian Vacuum Solitons, Spiral-Time HLV Dynamics, RAPS Coherence Architecture):

https://zenodo.org/records/17848351

Book download (Zenodo record):

https://zenodo.org/records/17849083

---

Contact

Don Michael Feeney Jr.

dfeen87@gmail.com

---

License

MIT License. See LICENSE.
