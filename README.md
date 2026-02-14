<div align="center">

# RAPS-HLV Flight Middleware

**Advanced Safety & Predictive Intelligence Layer for Flight-Ready Systems**

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![CI](https://github.com/dfeen87/HLV-RAPS/actions/workflows/ci.yml/badge.svg)](https://github.com/dfeen87/HLV-RAPS/actions/workflows/ci.yml)
[![C++](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://isocpp.org/)
[![CMake](https://img.shields.io/badge/CMake-3.10+-064F8C.svg)](https://cmake.org/)
[![Platform](https://img.shields.io/badge/Platform-Linux%20%7C%20RTOS-lightgrey.svg)](https://github.com/dfeen87/HLV-RAPS)














*Powered by the Helix–Light–Vortex (HLV) Framework*

[Overview](#overview) •
[Features](#key-features) •
[Architecture](#architecture) •
[Quick Start](#quick-start) •
[Documentation](#documentation) •
[Contributing](#contributing) •
[Publications](#publications--references)

</div>

---

## ⚠️ Important Notice

This repository provides a **flight-safety architecture and reference implementation** intended for:
- Research and academic study
- Simulation and prototyping
- Engineering development and validation

**This software is NOT certified for operational flight use.** Production deployment requires a complete certification program including requirements traceability, verification, validation, hardware qualification, and regulatory compliance.

---

## 📋 Table of Contents

- [Overview](#overview)
- [Why HLV-RAPS?](#why-hlv-raps)
- [Key Features](#key-features)
- [Core Concepts](#core-concepts)
- [Architecture](#architecture)
  - [Predictive Digital Twin (PDT)](#1-predictive-digital-twin-pdt)
  - [Deterministic Safety Monitor (DSM)](#2-deterministic-safety-monitor-dsm)
  - [RAPS Governance Loop](#3-raps-governance-loop-zero-trust-execution)
  - [Immutable Telemetry Ledger (ITL)](#4-immutable-telemetry-ledger-itl)
  - [REST API](#5-rest-api-for-observability)
- [Quick Start](#quick-start)
- [Repository Structure](#repository-structure)
- [Documentation](#documentation)
- [Testing & Validation](#testing--validation)
  - [Software-in-the-Loop (SIL)](#software-in-the-loop-sil)
  - [Hardware-in-the-Loop (HIL)](#hardware-in-the-loop-hil)
- [Continuous Integration](#continuous-integration)
- [Contributing](#contributing)
- [Publications & References](#publications--references)
- [Authors](#authors)
- [License](#license)

---

## Overview

**HLV Flight Middleware** is a deterministic safety and predictive intelligence layer designed to operate between flight control computers (or avionics controllers) and mission-critical subsystems such as propulsion, power management, thermal control, actuation, and guidance systems.

### What It Does

The middleware transforms traditional reactive monitoring into a **proactive governed control loop**:

- 🔮 **Predict** — Forecast system behavior with uncertainty quantification
- ✅ **Validate** — Verify proposed actions against hard safety envelopes
- ⚡ **Execute** — Perform actions only when safe, with automatic fallback/rollback
- 📝 **Audit** — Maintain immutable telemetry for complete traceability

### Engineering Principles

Built on **flight-ready engineering practices**:
- ✓ Deterministic runtime behavior
- ✓ Bounded execution latency
- ✓ Multi-layer safety gating
- ✓ Built-in redundancy support
- ✓ Comprehensive rollback capabilities
- ✓ Immutable telemetry ledger

---

## Why HLV-RAPS?

Modern aerospace systems fail in the margins: small deviations accumulate across thermal stress, fatigue, timing drift, sensor noise, and coupled subsystem dynamics.

### Traditional Flight Software Limitations

- ❌ Detects problems **after** thresholds are crossed
- ❌ Lacks predictive gating at the decision layer
- ❌ Provides logs that are hard to trust or reconstruct
- ❌ Has limited rollback/failover primitives

### HLV-RAPS Solution

HLV Flight Middleware closes this gap by introducing:

- ✅ **Predictive Digital Twin** — Forecast with confidence/uncertainty estimation
- ✅ **Deterministic Safety Monitor** — Hard limits with independent checks
- ✅ **Governance Loop** — Sense → Predict → Validate → Act → Audit
- ✅ **Rollback + Redundancy** — A/B supervisory control with failover hooks
- ✅ **Immutable Telemetry Ledger (ITL)** — Merkle anchoring for auditable traces

---

## Key Features

### 🛡️ Safety & Reliability

- **Multi-layer safety validation** with independent monitors
- **Automatic rollback** on execution failure or deviation
- **Redundancy-ready** architecture with A/B supervisor support
- **Fail-safe behavior** with fallback safe-state restoration

### 🔮 Predictive Intelligence

- **Digital twin forecasting** with uncertainty quantification
- **Monte Carlo sampling** for risk assessment
- **Online residual learning** hooks for adaptive behavior
- **Early Safety Excursion (ESE)** signal prediction

### 📊 Observability & Audit

- **Immutable telemetry ledger** with Merkle anchoring
- **REST API** for real-time monitoring (observability-only)
- **Thread-safe data access** with mutex protection
- **Post-flight audit integrity** verification

### 🔧 Portability & Integration

- **Platform HAL abstraction** for hardware independence
- **RTOS mutex abstractions** for real-time systems
- **Build-system agnostic** (CMake-friendly)
- **Clear seams** for SIL/HIL testing

---

## Core Concepts

### Dual-State Modeling

The middleware models the system in two coupled layers:

- **Physical State (Ψ)**: Classical metrics (voltage, current, temperature, cycles, stress, position, velocity, etc.)
- **Informational State (Φ)**: Entropy, degradation history, anomaly geometry, drift, and coherence

### Mathematical Framework

Coupling is expressed through an effective metric:

```
g_μν^eff = g_μν + λ (∂_μ Φ)(∂_ν Φ)
```

**Practical Interpretation**: Φ acts as a *structured memory + distortion field* that influences how the middleware interprets "normal" evolution of Ψ, enabling earlier detection and better predictive safety gating.

---

## Architecture

### System Diagram

```
┌─────────────────────────────────────────────────────────────┐
│                    Flight Controller                         │
└───────────────────────────┬─────────────────────────────────┘
                            │
                            ▼
┌─────────────────────────────────────────────────────────────┐
│                  HLV-RAPS Middleware                         │
│  ┌──────────────┐  ┌──────────────┐  ┌─────────────────┐   │
│  │ Predictive   │  │ Deterministic│  │   Governance    │   │
│  │ Digital Twin │→ │    Safety    │→ │      Loop       │   │
│  │    (PDT)     │  │  Monitor(DSM)│  │     (RAPS)      │   │
│  └──────────────┘  └──────────────┘  └─────────────────┘   │
│                                                              │
│  ┌──────────────────────────────────────────────────────┐   │
│  │      Immutable Telemetry Ledger (ITL)                │   │
│  │           + REST API Server                          │   │
│  └──────────────────────────────────────────────────────┘   │
└───────────────────────────┬─────────────────────────────────┘
                            │
                            ▼
┌─────────────────────────────────────────────────────────────┐
│   Mission-Critical Subsystems                                │
│   (Propulsion, Power, Thermal, Actuation, Guidance)         │
└─────────────────────────────────────────────────────────────┘
```

### 1) Predictive Digital Twin (PDT)

The PDT forecasts near-future system state under candidate commands/policies:

**Outputs:**
- Predicted end state
- Confidence & uncertainty metrics
- Predicted Early Safety Excursion (ESE) signals

**Implementations:**
- Deterministic step simulation
- Monte Carlo sampling for uncertainty estimation
- Optional online residual learning hooks

### 2) Deterministic Safety Monitor (DSM)

An independent safety layer enforcing **inviolable bounds**.

**Capabilities:**
- Demand rollback (safe abort)
- Force full shutdown (catastrophic prevention)
- Restore safe-state

**Design Philosophy**: Intentionally simple, conservative, and suitable for separation on independent compute/sensor channels.

### 3) RAPS Governance Loop (Zero-Trust Execution)

A deterministic orchestrator implementing:

1. **SENSE & AUDIT** — Snapshot + commit to ITL
2. **PREDICT & PLAN** — Generate candidate policies
3. **VALIDATE** — AILEE safety gates + DSM checks
4. **EXECUTE** — Idempotent actuator transaction
5. **ROLLBACK** — If execution fails or integrity breaks
6. **AUDIT** — Immutable telemetry + Merkle anchoring

### 4) Immutable Telemetry Ledger (ITL)

A compact embedded ledger providing:

- Event commits with cryptographic hashing
- Merkle batching for efficient verification
- Root anchoring for post-flight audit integrity
- Optional downlink mirroring

### 5) REST API for Observability

A lightweight read-only HTTP/JSON API:

- **Observability-only** (GET endpoints, no control surfaces)
- **Network accessible** (binds to 0.0.0.0:8080)
- **Non-blocking** (dedicated thread, mutex-protected)
- **Real-time snapshots** of PDT, DSM, Supervisor, Rollback, ITL, and state

📖 **Documentation**: See [`docs/REST_API.md`](docs/REST_API.md) for endpoint details and [`examples/api_client/`](examples/api_client/) for Python client examples.

---

## Quick Start

### Prerequisites

- **C++ Compiler** with C++17 support (GCC 7+, Clang 5+, MSVC 2017+)
- **CMake** 3.10 or higher
- **Build Tools** (make, ninja, or platform equivalent)

### Building the Demo

```bash
# Clone the repository
git clone https://github.com/dfeen87/HLV-RAPS.git
cd HLV-RAPS

# Build the Software-in-the-Loop (SIL) test harness
cmake -S tests/sil -B build
cmake --build build

# Run tests
ctest --test-dir build --output-on-failure
```

### Running the RTOS Demo

```bash
# Build and run the RTOS demonstration
# (Adjust toolchain/platform as needed for your target)
./build/hlv_rtos_demo
```

### Running the REST API Server

```bash
# Build and run the API server demo
./build/rest_api_demo

# In another terminal, access the API
curl http://localhost:8080/api/status
```

📖 **For detailed build instructions**, see the repository documentation in `docs/`.

---

## Repository Structure

```
HLV-RAPS/
├── docs/                           # Documentation
│   ├── REST_API.md                # REST API endpoint reference
│   ├── architecture.md            # Detailed architecture guide
│   ├── sil_hil.md                # SIL/HIL testing documentation
│   └── verification.md           # Verification approach
│
├── examples/                       # Example applications
│   ├── hlv_demo/                  # RTOS demonstration
│   └── api_client/                # REST API client examples
│
├── include/                        # Public API headers
│   ├── apcu/                      # Advanced Propulsion Control Unit
│   ├── config/                    # Configuration and safety limits
│   ├── core/                      # Core definitions and types
│   ├── hlv/                       # HLV framework headers
│   ├── itl/                       # Immutable Telemetry Ledger
│   ├── platform/                  # Platform abstraction layer
│   └── raps/                      # RAPS core components
│       ├── api/                   # REST API interfaces
│       ├── core/                  # RAPS core types
│       ├── hlv/                   # HLV field dynamics
│       ├── pdt/                   # Predictive Digital Twin
│       ├── platform/              # RTOS abstractions
│       ├── safety/                # Safety monitors
│       └── supervisor/            # Redundant supervisor
│
├── src/                            # Implementation sources
│   ├── control/                   # Control algorithms
│   ├── hlv/                       # HLV subsystem modules
│   ├── itl/                       # ITL primitives
│   ├── physics/                   # Physics engines
│   ├── raps/                      # RAPS implementation
│   └── supervisor/                # Supervisor logic
│
├── tests/                          # Test infrastructure
│   └── sil/                       # Software-in-the-Loop tests
│
├── tools/                          # Development tools
├── reference/                      # Reference implementations
│
├── LICENSE                         # MIT License
└── README.md                       # This file
```

### Key Components

| Directory | Description |
|-----------|-------------|
| `include/raps/pdt/` | Predictive Digital Twin engine |
| `include/raps/safety/` | Deterministic Safety Monitor |
| `include/raps/supervisor/` | Redundant supervisor and failover |
| `include/itl/` | Immutable Telemetry Ledger |
| `include/raps/api/` | REST API server and snapshots |
| `src/hlv/` | HLV subsystem modules (gravity control, time dilation, etc.) |
| `examples/hlv_demo/` | RTOS demonstration harness |
| `tests/sil/` | Software-in-the-Loop test suite |

---

## Documentation

Comprehensive documentation is available in the [`docs/`](docs/) directory:

| Document | Description |
|----------|-------------|
| [`architecture.md`](docs/architecture.md) | Detailed system architecture and design |
| [`REST_API.md`](docs/REST_API.md) | REST API endpoint reference |
| [`sil_hil.md`](docs/sil_hil.md) | SIL/HIL testing methodology |
| [`verification.md`](docs/verification.md) | Verification and validation approach |
| [`operational_notes.md`](docs/operational_notes.md) | Operational considerations |

---

## Testing & Validation

RAPS is designed to be **flight-ready by construction**, not by late-stage testing. The repository includes first-class SIL and HIL infrastructure.

### Software-in-the-Loop (SIL)

**Purpose**: Deterministic, CI-friendly execution environment for the full RAPS control, prediction, and safety pipeline.

**Key Capabilities**:
- ✓ PlatformHAL SIL backend (target-agnostic stubs)
- ✓ Compile-time fault injection (actuator failures, flash errors, latency spikes)
- ✓ Deterministic execution mode (reproducible test runs)
- ✓ Coverage gates (rollback paths, failover, safety monitor responses)

**Running SIL Tests**:

```bash
cmake -S tests/sil -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

**Why It Matters**: Validates governance logic, catches race conditions and rollback bugs early, makes safety behavior auditable and repeatable.

### Hardware-in-the-Loop (HIL)

**Purpose**: Validate software assumptions against real hardware, timing, and actuator constraints.

**Key Capabilities**:
- ✓ HIL-backed PlatformHAL (hardware/rig-server implementation)
- ✓ Rig-driven actuator validation
- ✓ Downlink and telemetry verification
- ✓ One-shot readiness check (`examples/hil/hil_readiness_check.cpp`)

**Why It Matters**: Proves software can survive real latencies and failures, prevents "it worked in simulation" surprises.

### Unified Design Philosophy

SIL and HIL are **link-time personalities** of the same system:
- Same governance logic runs in SIL, HIL, and flight
- Safety and rollback behavior validated before certification
- Flight builds simply replace PlatformHAL backend with certified drivers

This enables seamless progression: **Research → Simulation → Hardware → Flight** without rewriting core logic.

---

## Continuous Integration

### What CI Validates

- ✓ Configures and builds the SIL test harness
- ✓ Runs fault-injection smoke tests
- ✓ Validates deterministic safety behavior

### What CI Does NOT Check

- Hardware-in-the-Loop (HIL) validation
- Physical testbed integration
- Network-dependent external services

### Reproducing CI Locally

```bash
cmake -S tests/sil -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

---

## Contributing

We welcome contributions! This project is MIT licensed and open to:

- 🔍 **Audits and reviews** — Security, safety, and code quality
- 🛡️ **Safety envelope expansions** — Additional safety constraints and validation
- 🔧 **Portability improvements** — RTOS/HAL adaptations for new platforms
- 🧪 **Test harnesses** — SIL/HIL test scenarios and coverage expansion
- 📚 **Documentation** — Guides, diagrams, tutorials, and specification alignment
- 🐛 **Bug reports and fixes** — Issue reporting and resolution

### How to Contribute

1. **Fork** the repository
2. **Create** a feature branch (`git checkout -b feature/amazing-feature`)
3. **Commit** your changes (`git commit -m 'Add amazing feature'`)
4. **Push** to the branch (`git push origin feature/amazing-feature`)
5. **Open** a Pull Request

### Guidelines

- Follow existing code style and conventions
- Add tests for new features
- Update documentation as needed
- Ensure all tests pass before submitting PR
- Provide clear commit messages and PR descriptions

---

## Publications & References

[![DOI](https://zenodo.org/badge/DOI/10.5281/zenodo.17848351.svg)](https://doi.org/10.5281/zenodo.17848351)
[![DOI](https://zenodo.org/badge/DOI/10.5281/zenodo.17849083.svg)](https://doi.org/10.5281/zenodo.17849083)

### Primary Documentation

**RAPS Foundational Document + Part II (HLV Physics Math Implementation)**
- 📖 [AI-Augmented Rigor: A Zero-Trust Governance Architecture](https://dfeen.substack.com/p/ai-augmented-rigor-a-zero-trust-governance)

### Academic Publications

**Preprint: Gaussian Vacuum Solitons, Spiral-Time HLV Dynamics, RAPS Coherence Architecture**
- 📄 [Zenodo Record 17848351](https://zenodo.org/records/17848351) • [![DOI](https://zenodo.org/badge/DOI/10.5281/zenodo.17848351.svg)](https://doi.org/10.5281/zenodo.17848351)

**Book Download**
- 📚 [Zenodo Record 17849083](https://zenodo.org/records/17849083) • [![DOI](https://zenodo.org/badge/DOI/10.5281/zenodo.17849083.svg)](https://doi.org/10.5281/zenodo.17849083)

### Citation

If you use this work in academic research, please cite:

```bibtex
@software{hlv_raps_2025,
  author = {Feeney Jr., Don Michael and Krüger, Marcel},
  title = {RAPS-HLV Flight Middleware: Advanced Safety \& Predictive Intelligence Layer},
  year = {2025},
  publisher = {GitHub},
  url = {https://github.com/dfeen87/HLV-RAPS}
}
```

---

## Authors

**Don Michael Feeney Jr.**
- Email: dfeen87@gmail.com
- Primary architect and developer

**Marcel Krüger**
- Co-author and contributor

### Acknowledgments

This project builds upon research in aerospace safety systems, predictive control, and flight middleware architectures. We thank the broader aerospace and computer science communities for their foundational work.

---

## License

This project is licensed under the **MIT License** - see the [LICENSE](LICENSE) file for details.

```
MIT License

Copyright (c) 2025 Don Michael Feeney Jr

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

---

<div align="center">

**[⬆ Back to Top](#raps-hlv-flight-middleware)**

Made with ❤️ for flight safety and aerospace innovation

</div>
