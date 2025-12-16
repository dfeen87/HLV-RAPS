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

