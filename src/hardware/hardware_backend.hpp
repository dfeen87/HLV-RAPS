#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <chrono>
#include <functional>

namespace apms::hw {

// -----------------------------
// Status / Errors
// -----------------------------
enum class Status : uint8_t {
    Ok = 0,
    NotReady,
    Timeout,
    EndOfStream,
    InvalidArgument,
    Unsupported,
    IoError,
    InternalError
};

inline const char* to_string(Status s) noexcept {
    switch (s) {
        case Status::Ok:              return "Ok";
        case Status::NotReady:        return "NotReady";
        case Status::Timeout:         return "Timeout";
        case Status::EndOfStream:     return "EndOfStream";
        case Status::InvalidArgument: return "InvalidArgument";
        case Status::Unsupported:     return "Unsupported";
        case Status::IoError:         return "IoError";
        case Status::InternalError:   return "InternalError";
        default:                      return "Unknown";
    }
}

// -----------------------------
// Time Types
// -----------------------------
using SteadyClock   = std::chrono::steady_clock;
using TimePoint     = SteadyClock::time_point;
using Nanoseconds   = std::chrono::nanoseconds;

// -----------------------------
// Audio Types
// -----------------------------
// Interleaved floating-point samples:
// [frame0_ch0, frame0_ch1, ..., frame0_chN, frame1_ch0, ...]
struct AudioFormat {
    uint32_t sample_rate_hz   = 48000;
    uint16_t channels         = 1;
    uint16_t frames_per_block = 480;  // e.g., 10ms @ 48kHz
};

struct AudioBlock {
    AudioFormat format{};
    std::vector<float> samples; // size = frames_per_block * channels

    // Monotonic timing for this block (when first sample was captured / should be played)
    TimePoint timestamp{};
    // Optional sequence counter for debugging / continuity
    uint64_t sequence = 0;

    void resize_for_format() {
        samples.resize(static_cast<size_t>(format.frames_per_block) * format.channels);
    }

    bool valid_shape() const noexcept {
        return samples.size() ==
               static_cast<size_t>(format.frames_per_block) * format.channels;
    }
};

// -----------------------------
// Backend Capabilities
// -----------------------------
enum class Capability : uint32_t {
    AudioInput            = 1u << 0,
    AudioOutput           = 1u << 1,
    ClockSync             = 1u << 2, // backend can align capture/play timestamps to hardware clock
    LowLatencyHint        = 1u << 3,
    DeviceEnumeration     = 1u << 4,
    ExternalTriggerInput  = 1u << 5, // e.g., GPIO trigger / PPS
    ExternalTriggerOutput = 1u << 6  // e.g., strobe / sync out
};

inline constexpr uint32_t cap_mask(Capability c) noexcept {
    return static_cast<uint32_t>(c);
}

// -----------------------------
// Configuration
// -----------------------------
struct DeviceSelector {
    // If empty, backend chooses sensible default device.
    std::string input_device_id;
    std::string output_device_id;
};

struct BackendConfig {
    AudioFormat input_format{};
    AudioFormat output_format{};
    DeviceSelector devices{};

    // Operational hints (backend may ignore if unsupported)
    bool prefer_low_latency = true;
    bool exclusive_mode     = false;   // some platforms support exclusive access for stable timing
    uint32_t target_latency_ms = 30;   // hint only

    // Optional: allow backends to attach custom config payloads (JSON, key=val, etc.)
    std::string opaque_options;
};

// -----------------------------
// Logging Hook (optional)
// -----------------------------
enum class LogLevel : uint8_t { Debug, Info, Warn, Error };

using LogSink = std::function<void(LogLevel, const std::string&)>;

// -----------------------------
// IHardwareBackend
// -----------------------------
//
// Design goals:
// - Minimal primitives for SIL/HIL: init/start/stop + read/write blocks.
// - Deterministic timing surface: steady clock timestamps.
// - No dependency on OS audio APIs in the interface.
// - Backends can represent real hardware, loopback devices, simulators, or recorded streams.
//
// Threading model (recommended):
// - read_input_block() called from acquisition thread
// - write_output_block() called from playback/projection thread
// - Implementations should be thread-safe for concurrent read/write calls.
//
class IHardwareBackend {
public:
    virtual ~IHardwareBackend() = default;

    // Human-friendly backend name (e.g., "portaudio", "coreaudio", "alsa", "wasapi", "sim")
    virtual std::string name() const = 0;

    // Bitmask of Capability flags
    virtual uint32_t capabilities() const noexcept = 0;

    // Provide a logging sink (optional). Backend should not assume sink is always set.
    virtual void set_log_sink(LogSink sink) { log_sink_ = std::move(sink); }

    // Initialize backend resources (device open, buffers, etc.)
    // Safe to call once; may return Unsupported if backend is not available on this platform.
    virtual Status initialize(const BackendConfig& cfg) = 0;

    // Start streaming / activation. After start():
    // - read_input_block() and/or write_output_block() should become usable depending on caps.
    virtual Status start() = 0;

    // Stop streaming / deactivate. Safe to call multiple times.
    virtual Status stop() = 0;

    // Release all resources; after shutdown() backend may be re-initialized.
    virtual void shutdown() = 0;

    // Returns current monotonic time in backend domain. Default uses steady_clock directly.
    virtual TimePoint now() const noexcept { return SteadyClock::now(); }

    // Blocking read of one input block.
    // - If backend lacks AudioInput, returns Unsupported.
    // - If timeout is provided and expires, returns Timeout.
    // - EndOfStream is used for finite sources (file replay, scripted SIL runs).
    virtual Status read_input_block(AudioBlock& out,
                                    std::optional<Nanoseconds> timeout = std::nullopt) = 0;

    // Blocking write of one output block.
    // - If backend lacks AudioOutput, returns Unsupported.
    // - If timeout is provided and expires, returns Timeout.
    virtual Status write_output_block(const AudioBlock& in,
                                      std::optional<Nanoseconds> timeout = std::nullopt) = 0;

    // Optional: enumerate devices. If unsupported, return empty list.
    // Device IDs are backend-defined but must be stable within a run.
    struct DeviceInfo {
        std::string id;
        std::string label;
        bool is_default = false;
    };

    virtual std::vector<DeviceInfo> list_input_devices()  { return {}; }
    virtual std::vector<DeviceInfo> list_output_devices() { return {}; }

protected:
    void log_(LogLevel lvl, const std::string& msg) const {
        if (log_sink_) log_sink_(lvl, msg);
    }

    LogSink log_sink_{};
};

// -----------------------------
// Simple Registry / Factory
// -----------------------------
//
// Purpose:
// - Enables selecting backends by name from config/CLI.
// - Keeps the core independent from specific backend link targets.
//
class BackendRegistry {
public:
    using Factory = std::function<std::unique_ptr<IHardwareBackend>()>;

    static BackendRegistry& instance() {
        static BackendRegistry r;
        return r;
    }

    // Register a backend factory under a stable name.
    // If name already exists, it will be overwritten (last-writer wins).
    void register_factory(std::string backend_name, Factory factory) {
        entries_.push_back({std::move(backend_name), std::move(factory)});
    }

    // Create backend by name. Returns nullptr if not found.
    std::unique_ptr<IHardwareBackend> create(const std::string& backend_name) const {
        for (const auto& e : entries_) {
            if (e.name == backend_name) return e.factory();
        }
        return nullptr;
    }

    // List available backend names (in registration order).
    std::vector<std::string> available() const {
        std::vector<std::string> names;
        names.reserve(entries_.size());
        for (const auto& e : entries_) names.push_back(e.name);
        return names;
    }

private:
    BackendRegistry() = default;

    struct Entry {
        std::string name;
        Factory factory;
    };
    std::vector<Entry> entries_;
};

// Convenience macro for static registration (optional).
// Use in a .cpp:
//   static apms::hw::BackendAutoRegister reg{"sim", [](){ return std::make_unique<SimBackend>(); }};
struct BackendAutoRegister {
    BackendAutoRegister(std::string backend_name, BackendRegistry::Factory factory) {
        BackendRegistry::instance().register_factory(std::move(backend_name), std::move(factory));
    }
};

} // namespace apms::hw
