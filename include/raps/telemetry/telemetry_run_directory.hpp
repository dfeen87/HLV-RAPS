#pragma once
/*
 * RAPS Telemetry Run Directory Helper
 *
 * Lightweight utility for creating timestamped run directories.
 * Safe, non-blocking, and tolerant of failure.
 *
 * Directory structure:
 *   data/telemetry/runs/run_YYYY-MM-DDTHH-MM-SSZ/
 *   data/telemetry/runs/latest -> run_YYYY-MM-DDTHH-MM-SSZ
 *
 * License: MIT
 */

#include <string>
#include <string_view>
#include <ctime>
#include <cstdio>
#include <sys/stat.h>
#include <unistd.h>
#include <cstring>

namespace raps::telemetry {

// Generate ISO 8601 UTC timestamp (filesystem-safe)
inline std::string iso_utc_now() noexcept {
    std::time_t t = std::time(nullptr);
    if (t == static_cast<std::time_t>(-1)) {
        // Fallback to epoch on error
        return "1970-01-01T00-00-00Z";
    }
    
    std::tm tm{};
    if (!gmtime_r(&t, &tm)) {
        return "1970-01-01T00-00-00Z";
    }
    
    char buf[32];
    const size_t written = std::strftime(buf, sizeof(buf), 
                                         "%Y-%m-%dT%H-%M-%SZ", &tm);
    
    if (written == 0) {
        return "1970-01-01T00-00-00Z";
    }
    
    return std::string(buf, written);
}

// Create directory if it doesn't exist (best-effort)
// Returns true if directory exists (created or already present)
inline bool mkdir_if_missing(const char* path) noexcept {
    if (!path || path[0] == '\0') {
        return false;
    }
    
    struct stat st;
    if (stat(path, &st) == 0) {
        // Path exists - check if it's a directory
        return S_ISDIR(st.st_mode);
    }
    
    // Create directory with rwxr-xr-x permissions
    return mkdir(path, 0755) == 0;
}

// Create timestamped run directory with full hierarchy
// Returns the run directory path on success, empty string on failure
// Failure to create directories is NOT fatal - caller should handle gracefully
inline std::string create_run_directory() noexcept {
    // Create hierarchy step-by-step
    if (!mkdir_if_missing("data")) {
        return "";
    }
    
    if (!mkdir_if_missing("data/telemetry")) {
        return "";
    }
    
    if (!mkdir_if_missing("data/telemetry/runs")) {
        return "";
    }
    
    // Generate timestamped directory name
    const std::string timestamp = iso_utc_now();
    const std::string run_dir = 
        std::string("data/telemetry/runs/run_") + timestamp;
    
    if (!mkdir_if_missing(run_dir.c_str())) {
        return "";
    }
    
    // Best-effort "latest" symlink update
    // Failure here is non-critical
    const char* latest_link = "data/telemetry/runs/latest";
    
    // Remove existing symlink (ignore errors)
    unlink(latest_link);
    
    // Create new symlink (relative path for portability)
    const std::string relative_target = std::string("run_") + timestamp;
    symlink(relative_target.c_str(), latest_link);
    
    return run_dir;
}

// Validate that a run directory exists and is writable
inline bool validate_run_directory(std::string_view run_dir) noexcept {
    if (run_dir.empty()) {
        return false;
    }
    
    struct stat st;
    if (stat(run_dir.data(), &st) != 0) {
        return false;
    }
    
    if (!S_ISDIR(st.st_mode)) {
        return false;
    }
    
    // Check write permission (best-effort)
    return access(run_dir.data(), W_OK) == 0;
}

// Get the latest run directory path (follows symlink)
// Returns empty string if not found or not a directory
inline std::string get_latest_run_directory() noexcept {
    const char* latest_link = "data/telemetry/runs/latest";
    
    char resolved_path[1024];
    if (!realpath(latest_link, resolved_path)) {
        return "";
    }
    
    struct stat st;
    if (stat(resolved_path, &st) != 0 || !S_ISDIR(st.st_mode)) {
        return "";
    }
    
    return std::string(resolved_path);
}

// Format helpers for file paths within run directory
inline std::string telemetry_jsonl_path(std::string_view run_dir) noexcept {
    if (run_dir.empty()) return "";
    return std::string(run_dir) + "/telemetry.jsonl";
}

inline std::string summary_txt_path(std::string_view run_dir) noexcept {
    if (run_dir.empty()) return "";
    return std::string(run_dir) + "/summary.txt";
}

inline std::string meta_json_path(std::string_view run_dir) noexcept {
    if (run_dir.empty()) return "";
    return std::string(run_dir) + "/meta.json";
}

} // namespace raps::telemetry
