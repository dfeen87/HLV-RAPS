# Software-in-the-Loop (SIL) and Hardware-in-the-Loop (HIL)

This document describes how Software-in-the-Loop (SIL) and Hardware-in-the-Loop (HIL) are used in this project, what each mode validates, and how both integrate with the system architecture and verification strategy.

SIL and HIL are treated as **execution modes**, not separate systems.

---

## 1. Design Intent

The SIL/HIL strategy is designed to ensure that:

- The **same core code paths** execute in all modes
- Differences between SIL and HIL are limited to the hardware backend
- Safety, policy, and profiling behave identically in both environments
- Verification evidence can be accumulated incrementally

There are **no SIL-only or HIL-only control branches** in the core system.

---

## 2. Definitions

### 2.1 Software-in-the-Loop (SIL)

SIL runs the full system using:

- The real core logic
- The real policy evaluation
- A simulated or reference hardware backend

SIL emphasizes:
- Determinism
- Replayability
- Fast iteration
- Regression detection

### 2.2 Hardware-in-the-Loop (HIL)

HIL runs the full system using:

- The same core logic
- The same policy evaluation
- A backend connected to real or semi-real hardware

HIL emphasizes:
- Timing behavior
- Integration correctness
- Real device constraints
- Environmental effects

---

## 3. Architectural Placement

SIL and HIL differ **only** at the hardware abstraction boundary:

        +---------------------+
        |   Core Processing   |
        |   Policy Evaluation |
        |   Profiling         |
        +----------+----------+
                   |
           IHardwareBackend
                   |
    +--------------+--------------+
    |                             |
Reference Backend Real Backend
(SIL) (HIL)


This guarantees that:
- Safety and policy logic are exercised identically
- Timing and performance differences are observable and measurable
- Verification artifacts remain comparable

---

## 4. Reference Backend (SIL)

The reference backend (`src/hardware/reference_backend.cpp`) exists to support SIL and early HIL scaffolding.

### 4.1 Supported Modes

The reference backend supports deterministic modes configured via `BackendConfig::opaque_options`:

| Mode        | Description |
|-------------|------------|
| `silence`   | Produces zeroed input blocks |
| `sine`      | Generates a deterministic sine wave |
| `loopback`  | Routes output blocks back to input with configurable delay |

### 4.2 Timing Simulation

The backend can simulate:

- Fixed latency (block-quantized)
- Optional jitter (block-quantized)
- End-of-stream termination for finite replays

This enables:
- Boundary testing
- Timeout handling validation
- Policy behavior under degraded timing

### 4.3 Determinism Guarantees

When jitter is disabled:
- Identical inputs produce identical outputs
- Replay runs are stable across executions
- Regression testing is straightforward

---

## 5. Typical SIL Use Cases

SIL is well-suited for:

- Policy boundary validation
- Regression testing
- Performance baselining
- Failure mode exploration
- Replay-based debugging

Example scenarios:
- Verifying slew-rate limits clamp correctly
- Ensuring duration limits escalate as expected
- Measuring control loop timing under load

---

## 6. HIL Integration

HIL backends implement the same `IHardwareBackend` interface and replace the reference backend at runtime.

### 6.1 What HIL Validates

HIL focuses on:

- Real device timing behavior
- Driver and transport stability
- Buffer alignment and format integrity
- Interaction with OS scheduling and load

### 6.2 What HIL Does *Not* Change

In HIL:
- Core algorithms remain unchanged
- Policy evaluation remains unchanged
- Profiling instrumentation remains unchanged

Only the backend implementation differs.

---

## 7. Profiling in SIL and HIL

The profiling layer operates identically in both modes.

Metrics commonly collected include:
- Control loop period
- Stage execution durations
- Latency distributions
- Jitter relative to declared targets

This allows:
- Direct comparison between SIL and HIL runs
- Identification of hardware-specific timing effects
- Evidence-based tuning

---

## 8. Failure Handling and Diagnostics

Both SIL and HIL support:

- Explicit `NotReady`, `Timeout`, and `EndOfStream` signaling
- Clean startup and shutdown transitions
- Controlled injection of failure conditions (SIL)

Failures are surfaced to the caller rather than hidden or auto-recovered, enabling explicit policy decisions.

---

## 9. Relationship to Verification

SIL and HIL provide complementary evidence:

| Aspect | SIL | HIL |
|------|-----|-----|
| Determinism | ✔️ | ✖️ |
| Replay | ✔️ | ✖️ |
| Timing realism | ✖️ | ✔️ |
| Hardware validation | ✖️ | ✔️ |
| Policy logic | ✔️ | ✔️ |

Together they support the verification properties defined in:
- `docs/verification.md`

---

## 10. Extension Guidelines

When adding new features:

- Prefer validating logic in SIL first
- Use HIL only after behavior is well understood
- Avoid introducing backend-specific conditionals
- Treat backend changes as integration changes, not logic changes

If SIL and HIL results diverge, this should be investigated and documented, not silently normalized.

---

## 11. Summary

SIL and HIL are first-class execution modes built into the system by design.

By isolating hardware interaction behind a strict abstraction boundary, the system achieves:

- High confidence through deterministic testing
- Real-world validation without code duplication
- A clear path from simulation to deployment

This approach supports both rapid development and high-assurance operation.

