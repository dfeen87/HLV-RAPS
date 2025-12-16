# Verification & Certification Tooling Plan

This document defines verifiable system properties, evidence artifacts, and a tooling roadmap for progressively increasing assurance in this codebase.

The goal is to make verification *routine* and *repeatable*:
- **SIL** (Software-in-the-Loop) provides deterministic, replayable correctness testing.
- **HIL** (Hardware-in-the-Loop) validates timing, device behavior, and integration stability.
- **Policy** expresses safety and mission constraints as explicit contracts.
- **Profiling** provides quantitative evidence for performance, jitter, and latency.

> Scope note: This document specifies *what must be proven* and *how evidence is collected*. It does not prescribe any single vendor toolchain.

---

## 1. Assurance Goals

### 1.1 Primary assurance objectives
1. **Safety envelope integrity**  
   Control and output behaviors remain within declared policy limits.
2. **Deterministic execution under SIL**  
   Given identical inputs/config, system produces identical outputs and event traces.
3. **Bounded latency and jitter**  
   Execution timing stays within declared budgets under defined load.
4. **Backend isolation**  
   Hardware specifics remain behind `IHardwareBackend` without leaking into core logic.
5. **Traceable evidence**  
   Every meaningful claim maps to a test, metric, or static analysis artifact.

### 1.2 Non-goals (for now)
- Proving correctness of third-party OS drivers or hardware firmware.
- Absolute real-time guarantees on non-real-time operating systems.
- Full formal proofs for every floating-point DSP component (unless explicitly required).

---

## 2. System Boundaries & Trust Model

### 2.1 Trust boundaries
- **Trusted**
  - Core algorithm code (target of verification)
  - Policy declarations (`src/policy/mission_policy.hpp`)
  - Profiling instrumentation (`src/profiling/perf_profiler.hpp`)
- **Partially trusted**
  - Reference backend (`src/hardware/reference_backend.cpp`) for deterministic SIL/HIL scaffolding
- **Untrusted / external**
  - OS scheduling, system load, physical hardware devices, vendor SDKs

### 2.2 Hardware abstraction boundary (critical invariant)
All device I/O must occur only via:
- `apms::hw::IHardwareBackend`
- `read_input_block(...)`
- `write_output_block(...)`

This enables:
- backends to be swapped without affecting core logic
- test harnesses to validate safety behaviors independently of hardware

---

## 3. Verifiable Properties

This section lists properties the system should maintain, expressed in a form suitable for tests, static analysis, and/or formal methods.

### 3.1 Policy compliance (Safety contract)
**Property P1 — Scalar envelope:**  
For every constrained scalar value `x`, if `ScalarLimit(min,max)` is active, then:
- `min ≤ x ≤ max` always holds after enforcement.

**Property P2 — Slew-rate:**  
For any value `x(t)` subject to `SlewRateLimit(max_delta_per_sec)`:
- `|x(t) - x(t-Δt)| / Δt ≤ max_delta_per_sec` for all evaluation steps.

**Property P3 — Duration constraint:**  
If a monitored condition persists continuously for duration `d`:
- `d ≤ max_duration` or the system escalates according to severity.

**Evidence sources:**
- SIL replay tests asserting clamp/escalation outcomes
- HIL tests verifying policy enforcement under realistic timing
- Static analysis confirming policy evaluation has no hidden side effects

### 3.2 Determinism and replay integrity (SIL)
**Property D1 — Deterministic replay:**  
Given identical configuration, input sequences, and random seeds, the system produces:
- identical output sequences
- identical policy violation traces
- identical performance metric summaries within tolerance

**Property D2 — End-of-stream behavior:**  
Finite sources (replay runs) produce:
- a clean `EndOfStream` termination
- no deadlocks, and no partial blocks

**Evidence sources:**
- Reference backend deterministic modes (`silence`, `sine`, `loopback`)
- Replay harness tests (if present) that compare output hashes and event logs

### 3.3 Timing budgets (Profiling + HIL)
**Property T1 — Loop period bound:**  
If target period is `T_target`, then:
- observed period deviation (jitter) stays below declared threshold `J_max` in specified environments.

**Property T2 — Stage execution bound:**  
For each profiled stage `S`, measured duration stays within:
- `S_max` under defined load profiles.

**Evidence sources:**
- `apms::profiling::Profiler` snapshots (min/max/mean)
- HIL runs on representative hardware with load injection

### 3.4 Backend isolation and interface soundness
**Property B1 — No backend leakage:**  
Core code must not directly include OS/device SDK headers. Backend-specific code must live in backend implementation units.

**Property B2 — Format integrity:**  
For any `AudioBlock` transmitted across the backend boundary:
- `samples.size() == frames_per_block * channels`

**Evidence sources:**
- Compile-time include checks / lint rules
- Unit tests validating format checks reject malformed buffers

---

## 4. Evidence Artifacts

The system should produce verifiable artifacts that can be archived per release:

### 4.1 Test evidence
- SIL test logs (inputs, outputs, policy events)
- HIL test logs (device configuration, timing stats, failures)
- Regression hashes (output block hashes or summary hashes)

### 4.2 Performance evidence
- Profiler snapshots:
  - sample count
  - min/max/mean duration
  - period jitter stats (if configured)
- Environment metadata:
  - CPU, OS, compiler version, build flags

### 4.3 Static analysis evidence
- Compiler warnings treated as errors
- Sanitizer runs (as feasible):
  - AddressSanitizer
  - UndefinedBehaviorSanitizer
  - ThreadSanitizer (for concurrency paths)
- Linting reports (optional)

---

## 5. Tooling Roadmap

This section lays out incremental adoption steps. Each step must be reproducible and CI-friendly.

### Phase 1 — Baseline verification (recommended now)
- Build with strict warnings:
  - `-Wall -Wextra -Wpedantic` (or equivalent)
- Run SIL tests:
  - deterministic modes: `silence`, `sine`, `loopback`
  - validate EOS behavior if used
- Collect profiling snapshots:
  - control loop period metric
  - core stage metrics

### Phase 2 — Robustness & race detection
- Run sanitizers in dedicated build profiles
- Add concurrency stress tests:
  - multiple producer/consumer threads calling backend read/write
- Add load injection tests:
  - CPU load
  - artificial backend delays

### Phase 3 — Formal methods (selective, high value)
Focus formal work on *interfaces and invariants*, not all DSP math.

Recommended targets:
- Policy invariants (P1–P3) expressed as:
  - property tests / model checking on policy evaluation
- State machine correctness for mission phases:
  - phase transition rules
  - safe shutdown properties
- Buffer shape and bounds:
  - memory safety properties across backend boundary

---

## 6. Certification Alignment

This project is structured to align with common certification expectations by making safety constraints explicit and testable:

- **Separation of concerns**
  - hardware boundary (`IHardwareBackend`)
  - policy contract (`MissionPolicy`)
  - observability (`Profiler`)
- **Traceability**
  - each claim maps to a measurable artifact
- **Incremental assurance**
  - SIL → HIL → static analysis → selective formal verification

If this system is used in regulated contexts, this document should be extended with:
- hazard analysis and risk controls
- requirements traceability matrix (RTM)
- verification protocol templates
- release evidence checklist

---

## 7. Release Evidence Checklist (Template)

For each release intended to increase assurance, capture:

- [ ] Compiler + build flags recorded
- [ ] SIL runs executed (modes: silence/sine/loopback as applicable)
- [ ] HIL runs executed (hardware IDs + environment recorded)
- [ ] Profiling snapshot recorded (min/max/mean + jitter where configured)
- [ ] Sanitizers executed (if applicable)
- [ ] Known limitations updated (docs)
- [ ] Policy changes reviewed for regression impact

---

## 8. Known Limitations (Current)

- Real-time guarantees depend on OS scheduling and hardware.
- Reference backend loopback delay/jitter are block-quantized by design.
- Formal verification is targeted and incremental; not all numerical code is proven.

---

## 9. Appendix: Mapping to Code

- Hardware boundary:
  - `src/hardware/hardware_backend.hpp`
  - `src/hardware/reference_backend.cpp`
- Policy contracts:
  - `src/policy/mission_policy.hpp`
- Profiling / evidence collection:
  - `src/profiling/perf_profiler.hpp`
- Shared vocabulary:
  - `src/common/types.hpp`
