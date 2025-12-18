#pragma once
/*
 * RAPS Telemetry Metadata Writer
 *
 * Writes immutable run metadata (meta.json) at run creation time.
 * Best-effort only: failure is non-fatal and must never impact runtime.
 *
 * This file exists to support auditability, provenance, and reproducibility.
 *
 * Contract:
 * - Written once per run
 * - Never modified afterward
 * - Failure to write is acceptable
 * - No blocking in control paths
 *
 * License: MIT (see LICENSE)
 */

#include <cstdio>
#include <cstring>
#include <string>
#include <string_view>
#include <ctime>
#include <sys/stat.h>
#include <unistd.h>

namespace raps::telemetry {

namespace detail {

// Best-effort ISO 8601 UTC timestamp
inline std::string iso_utc_now() noexcept {
    std::time_t t = std::time(nullptr);
    if (t == static_cast<std::time_t>(-1)) {
        return "1970-01-01T00:00:00Z";
    }

    std::tm tm{};
    if (!gmtime_r(&t, &tm)) {
        return "1970-01-01T00:00:00Z";
    }

    char buf[32];
    if (std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &tm) == 0) {
        return "1970-01-01T00:00:00Z";
    }

    return std::string(buf);
}

// Check if a file already exists
inline bool file_exists(const char* path) noexcept {
    struct stat st;
    return (path && stat(path, &st) == 0 && S_ISREG(st.st_mode));
}

// Minimal JSON escaping (strings only)
inline void json_escape(FILE* f, const char* s) noexcept {
    if (!f || !s) return;
    for (const unsigned char* p = (const unsigned char*)s; *p; ++p) {
        switch (*p) {
            case '\\': std::fputs("\\\\", f); break;
            case '"':  std::fputs("\\\"", f); break;
            case '\n': std::fputs("\\n", f);  break;
            case '\r': std::fputs("\\r", f);  break;
            case '\t': std::fputs("\\t", f);  break;
            default:
                if (*p < 0x20)
                    std::fprintf(f, "\\u%04x", (unsigned int)(*p));
                else
                    std::fputc(*p, f);
        }
    }
}

} // namespace detail

// ----------------------------
// Metadata structure (optional)
// ----------------------------
//
// This struct is intentionally simple and POD-like.
// All fields are optional; empty strings are omitted.
//
struct TelemetryMetadata final {
    std::string raps_version;        // e.g. "2.3.0"
    std::string telemetry_schema;    // e.g. "1.0"
    std::string git_commit;          // optional
    std::string build_type;          // Debug / Release
    std::string compiler;            // e.g. gcc-13.2.0
    std::string os;                  // e.g. linux
    std::string arch;                // e.g. x86_64
    std::string notes;               // optional free-form notes
};

// ------------------------------------
// Write meta.json (best-effort, once)
// ------------------------------------
inline bool write_telemetry_metadata(
    std::string_view run_dir,
    const TelemetryMetadata& meta
) noexcept {
    if (run_dir.empty()) {
        return false;
    }

    const std::string path = std::string(run_dir) + "/meta.json";

    // Do not overwrite existing metadata
    if (detail::file_exists(path.c_str())) {
        return true; // already written; treat as success
    }

    FILE* f = std::fopen(path.c_str(), "wb");
    if (!f) {
        return false;
    }

    std::fputs("{\n", f);

    bool first = true;
    auto emit_field = [&](const char* key, const std::string& value) {
        if (value.empty()) return;
        if (!first) std::fputs(",\n", f);
        first = false;
        std::fprintf(f, "  \"%s\": \"", key);
        detail::json_escape(f, value.c_str());
        std::fputs("\"", f);
    };

    emit_field("raps_version", meta.raps_version);
    emit_field("telemetry_schema", meta.telemetry_schema);
    emit_field("git_commit", meta.git_commit);
    emit_field("build_type", meta.build_type);
    emit_field("compiler", meta.compiler);
    emit_field("os", meta.os);
    emit_field("arch", meta.arch);

    // runtime block (always included)
    if (!first) std::fputs(",\n", f);
    std::fputs("  \"runtime\": {\n", f);
    std::fprintf(f, "    \"start_time_utc\": \"%s\"\n",
                 detail::iso_utc_now().c_str());
    std::fputs("  }", f);
    first = false;

    if (!meta.notes.empty()) {
        std::fputs(",\n  \"notes\": \"", f);
        detail::json_escape(f, meta.notes.c_str());
        std::fputs("\"", f);
    }

    std::fputs("\n}\n", f);
    std::fclose(f);

    return true;
}

} // namespace raps::telemetry
