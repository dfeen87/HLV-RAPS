/*
 * RAPS telemetry_report
 * Minimal post-run report for JSONL telemetry.
 *
 * Build: g++ -O2 -std=c++17 tools/telemetry_report.cpp -o telemetry_report
 */

#include <cstdio>
#include <cstdint>
#include <cstring>

// Minimal line scanning; not a full JSON parser.
// We keep it simple: count types/severity and find dropped_total markers.

static bool contains(const char* line, const char* needle) {
  return std::strstr(line, needle) != nullptr;
}

int main(int argc, char** argv) {
  if (argc < 2) {
    std::fprintf(stderr, "usage: %s raps.telemetry.jsonl\n", argv[0]);
    return 2;
  }

  FILE* f = std::fopen(argv[1], "rb");
  if (!f) {
    std::fprintf(stderr, "failed to open: %s\n", argv[1]);
    return 2;
  }

  uint64_t total = 0;
  uint64_t dropped_total_seen = 0;

  uint64_t sev_debug=0, sev_info=0, sev_warn=0, sev_error=0, sev_fatal=0;
  uint64_t type_loop=0, type_gate=0, type_mode=0, type_input=0, type_msg=0, type_other=0;

  char buf[8192];
  while (std::fgets(buf, sizeof(buf), f)) {
    ++total;

    if (contains(buf, "\"type\":\"telemetry_summary\"")) {
      // crude extract: find "dropped_total":
      const char* p = std::strstr(buf, "\"dropped_total\":");
      if (p) {
        p += std::strlen("\"dropped_total\":");
        dropped_total_seen = (uint64_t)std::strtoull(p, nullptr, 10);
      }
      continue;
    }

    // severity
    if      (contains(buf, "\"severity\":\"debug\"")) ++sev_debug;
    else if (contains(buf, "\"severity\":\"info\""))  ++sev_info;
    else if (contains(buf, "\"severity\":\"warn\""))  ++sev_warn;
    else if (contains(buf, "\"severity\":\"error\"")) ++sev_error;
    else if (contains(buf, "\"severity\":\"fatal\"")) ++sev_fatal;

    // type
    if      (contains(buf, "\"type\":\"loop_timing\""))     ++type_loop;
    else if (contains(buf, "\"type\":\"safety_gate\""))     ++type_gate;
    else if (contains(buf, "\"type\":\"mode_transition\"")) ++type_mode;
    else if (contains(buf, "\"type\":\"input_metrics\""))   ++type_input;
    else if (contains(buf, "\"type\":\"message\""))         ++type_msg;
    else                                                    ++type_other;
  }

  std::fclose(f);

  std::printf("RAPS Telemetry Report\n");
  std::printf("--------------------\n");
  std::printf("File: %s\n", argv[1]);
  std::printf("Total lines: %llu\n", (unsigned long long)total);
  std::printf("Dropped total (latest summary): %llu\n\n", (unsigned long long)dropped_total_seen);

  std::printf("Severity counts\n");
  std::printf("  debug: %llu\n", (unsigned long long)sev_debug);
  std::printf("  info : %llu\n", (unsigned long long)sev_info);
  std::printf("  warn : %llu\n", (unsigned long long)sev_warn);
  std::printf("  error: %llu\n", (unsigned long long)sev_error);
  std::printf("  fatal: %llu\n\n", (unsigned long long)sev_fatal);

  std::printf("Event type counts\n");
  std::printf("  loop_timing     : %llu\n", (unsigned long long)type_loop);
  std::printf("  safety_gate     : %llu\n", (unsigned long long)type_gate);
  std::printf("  mode_transition : %llu\n", (unsigned long long)type_mode);
  std::printf("  input_metrics   : %llu\n", (unsigned long long)type_input);
  std::printf("  message         : %llu\n", (unsigned long long)type_msg);
  std::printf("  other           : %llu\n", (unsigned long long)type_other);

  return 0;
}
