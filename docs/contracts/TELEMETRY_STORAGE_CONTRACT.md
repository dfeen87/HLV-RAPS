# RAPS Telemetry Storage Contract

This document defines the **mandatory guarantees** and **behavioral contracts** for telemetry storage in RAPS.

These contracts ensure that telemetry remains strictly observational and never compromises runtime safety or determinism.

---

## Core Principles

### 1. Telemetry is Append-Only
- Once written, telemetry data is **never modified**
- No in-place updates to event logs
- No deletion of events within a run
- Files may only be appended or created, never overwritten

**Rationale**: Append-only semantics ensure audit integrity and simplify concurrent access patterns.

### 2. Each Run Gets Its Own Directory
- Every execution creates a unique, timestamped directory
- Format: `run_YYYY-MM-DDTHH-MM-SSZ`
- Directories are immutable after run completion
- No shared state between runs

**Rationale**: Isolation prevents cross-contamination and enables parallel analysis of historical runs.

### 3. Dashboards Read, Never Write
- Analysis tools are **strictly read-only**
- Dashboard output (e.g., `summary.txt`) is written once, post-run
- No runtime feedback from dashboard to RAPS
- No telemetry-driven control decisions

**Rationale**: Read-only access prevents tools from corrupting data or creating feedback loops.

### 4. Runtime Never Blocks on Storage
- Telemetry writes are **best-effort**
- I/O errors are logged but do not halt execution
- Ring buffer may drop events under load (tracked in `dropped_total`)
- No blocking syscalls in the control loop

**Rationale**: Storage failures must not compromise real-time guarantees or safety.

### 5. Failure to Write Telemetry Must Not Fail RAPS
- All telemetry operations are **non-fatal**
- Storage errors are handled gracefully
- RAPS continues operation even if telemetry is unavailable
- Missing telemetry is acceptable; crashing is not

**Rationale**: Telemetry is for observability, not control. System availability takes absolute priority.

---

## Directory Structure Contract

### Canonical Layout
```
data/
└── telemetry/
    ├── runs/
    │   ├── run_2025-12-18T14-22-09Z/
    │   │   ├── telemetry.jsonl
    │   │   ├── summary.txt
    │   │   └── meta.json
    │   ├── run_2025-12-18T15-30-45Z/
    │   │   ├── telemetry.jsonl
    │   │   ├── summary.txt
    │   │   └── meta.json
    │   └── latest -> run_2025-12-18T15-30-45Z
    └── README.md
```

### Path Guarantees
- `data/` is clearly non-source, non-config
- `telemetry/` has single responsibility: storing telemetry
- `runs/` contains immutable, timestamped subdirectories
- `latest` is a symbolic link to the most recent run (best-effort)

### File Naming
- Run directories: `run_YYYY-MM-DDTHH-MM-SSZ` (ISO 8601, UTC, filesystem-safe)
- Event log: `telemetry.jsonl` (JSON Lines format)
- Summary: `summary.txt` (human-readable text)
- Metadata: `meta.json` (structured JSON)

---

## File Format Contracts

### `telemetry.jsonl`
**Format**: JSON Lines (one JSON object per line)

**Requirements**:
- Each line is a valid JSON object
- No trailing commas
- UTF-8 encoding
- Unix line endings (`\n`)
- Deterministic ordering by `seq` field (best-effort)

**Schema Stability**: 
- Event schema is versioned (currently `1.0`)
- New fields may be added but existing fields are never removed
- Backward compatibility is maintained across minor versions

### `summary.txt`
**Format**: Plain text

**Requirements**:
- Human-readable ASCII or UTF-8
- Generated post-run by dashboard tools
- Optional (absence is not an error)

### `meta.json`
**Format**: JSON

**Requirements**:
- Valid JSON object
- Must include: `raps_version`, `telemetry_schema`, `build`, `runtime`
- Optional fields: `notes`, `build_timestamp`
- Written once at run creation

---

## Operational Contracts

### Storage Failure Modes

#### Acceptable Failures
- Disk full → drop events, record in `dropped_total`, continue
- Permission denied → log error, continue without telemetry
- Directory creation fails → fall back to `/tmp` or disable telemetry
- Symlink creation fails → continue without `latest` link

#### Unacceptable Failures
- Crash on I/O error
- Block control loop on write
- Corrupt existing telemetry files
- Leak file descriptors
- Throw unhandled exceptions

### Performance Requirements
- Telemetry must not add >5% overhead to control loop
- Writes must be buffered and non-blocking
- File handles must be managed efficiently
- Memory usage must be bounded (fixed ring buffer)

### Concurrency
- Multiple readers are always safe
- Single writer per run directory
- No file locking required for readers
- Writers use atomic operations or buffering

---

## Integration Requirements

### Runtime Integration
```cpp
// ✅ Correct: Non-blocking, failure-tolerant
const std::string run_dir = raps::telemetry::create_run_directory();
if (!run_dir.empty()) {
    const std::string path = run_dir + "/telemetry.jsonl";
    sink.open(path);  // Best-effort open
}
// RAPS continues regardless of success
```

```cpp
// ❌ Incorrect: Blocking, failure-intolerant
const std::string run_dir = raps::telemetry::create_run_directory();
assert(!run_dir.empty());  // Fatal if fails
sink.open(run_dir + "/telemetry.jsonl");
if (!sink.is_open()) {
    throw std::runtime_error("Cannot open telemetry");  // Fatal
}
```

### Dashboard Integration
```cpp
// ✅ Correct: Read-only access
void generate_summary(const std::string& run_dir) {
    std::ifstream events(run_dir + "/telemetry.jsonl");
    // ... analyze events ...
    std::ofstream summary(run_dir + "/summary.txt");  // Write once
    summary << results;
}
```

```cpp
// ❌ Incorrect: Modifies telemetry data
void filter_events(const std::string& run_dir) {
    // Remove low-priority events
    std::ofstream events(run_dir + "/telemetry.jsonl", std::ios::trunc);
    // ... write filtered events ...  // VIOLATION: modifies data
}
```

---

## Maintenance Contracts

### Retention Policy
- RAPS does not automatically delete old runs
- Operators are responsible for retention policies
- Consider automated cleanup after 30-90 days
- Archive critical runs before deletion

### Backup Strategy
- Run directories are self-contained
- Copy entire run directory for archival
- `meta.json` provides full provenance for reproduction

### Migration
- Run directories are portable across systems
- Relative paths are preferred (for `latest` symlink)
- Telemetry format is forward-compatible

---

## Validation Checklist

Before deployment, verify:

- [ ] Telemetry writes never block control loop
- [ ] Storage errors are caught and logged, not fatal
- [ ] Run directories are created with correct permissions
- [ ] `latest` symlink points to most recent run
- [ ] `meta.json` contains accurate build/runtime info
- [ ] Events are valid JSON Lines
- [ ] Dashboard tools do not modify run directories
- [ ] Memory usage is bounded (ring buffer)
- [ ] File descriptors are properly closed

---

## Support and Evolution

This contract is versioned and maintained alongside RAPS releases.

**Current version**: 1.0 (RAPS v2.3.0)

**Change policy**:
- Breaking changes require major version bump
- New fields may be added in minor versions
- Deprecations announced one release in advance

For questions or clarifications, refer to:
- `docs/telemetry_dashboard_v2.3.md`
- `data/telemetry/README.md`
- `src/telemetry/telemetry_run_dir.hpp`
