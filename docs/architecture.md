
Cross-cutting layers:

- **Profiling** — observes timing and performance
- **Verification** — defines what must be proven and how evidence is collected

---

## 3. Hardware Abstraction Layer

### 3.1 Purpose

All interaction with physical or simulated devices is isolated behind a single abstraction:

apms::hw::IHardwareBackend


This boundary ensures:

- Core logic does not depend on OS or hardware APIs
- SIL and HIL share the same execution path
- Hardware can be swapped without refactoring the system

### 3.2 Responsibilities

The hardware backend is responsible for:

- Device initialization and teardown
- Blocking I/O of time-stamped data blocks
- Reporting capability support
- Maintaining format integrity across the boundary

The core system assumes **nothing** about how these tasks are implemented.

### 3.3 Reference Backend

The reference backend provides:

- Deterministic SIL execution
- Loopback-based HIL scaffolding
- Configurable latency, jitter, and end-of-stream behavior

It exists to validate *system behavior*, not to model hardware perfectly.

---

## 4. Core Processing Layer

The core processing layer:

- Consumes input blocks from the backend
- Produces output blocks for projection
- Operates entirely in terms of abstract data structures
- Contains no direct hardware or policy enforcement code

This layer is intentionally kept small and explicit to simplify:

- Replay
- Debugging
- Formal reasoning

---

## 5. Policy Layer

### 5.1 Design Philosophy

Policies express **what must be true**, not **how to act**.

The policy layer is:

- Declarative
- Side-effect free
- Evaluated explicitly by the caller

### 5.2 Policy Types

Policies include:

- **Scalar limits** (safety envelopes)
- **Slew-rate limits** (rate-of-change constraints)
- **Duration limits** (time-based constraints)

Each policy returns a structured result indicating:

- Whether a violation occurred
- Severity (advisory → abort)
- Human-readable context

Enforcement decisions are made *outside* the policy layer.

---

## 6. Profiling and Observability

### 6.1 Principles

Profiling is designed to:

- Observe without influencing behavior
- Be optionally compiled out
- Produce deterministic, auditable metrics

### 6.2 Metrics

The profiling layer can capture:

- Stage execution durations
- Loop periods
- Min / max / mean statistics
- Jitter relative to declared targets

All metrics are collected using monotonic time and stored in a stable format suitable for logging or export.

---

## 7. SIL and HIL Integration

### 7.1 Software-in-the-Loop (SIL)

SIL execution uses:

- The same core logic
- The same policy evaluation
- A simulated backend

This enables:

- Deterministic replay
- Regression testing
- Policy boundary validation

### 7.2 Hardware-in-the-Loop (HIL)

HIL execution uses:

- The same core logic
- The same policy evaluation
- A real or semi-real backend

This validates:

- Timing behavior
- Integration stability
- Hardware interface assumptions

No code paths are exclusive to SIL or HIL.

---

## 8. Time and Type Consistency

All timing and common scalar types are centralized in:

