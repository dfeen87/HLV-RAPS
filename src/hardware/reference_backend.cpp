// src/hardware/reference_backend.cpp
//
// ReferenceBackend
// ----------------
// A deterministic, dependency-free reference implementation of IHardwareBackend.
// Intended for:
//   - SIL: simulation runs, replay harnesses, deterministic test vectors
//   - HIL scaffolding: a stable interface surface while real device backends mature
//
// Features (opt-in via BackendConfig::opaque_options):
//   - mode=silence        (default) produces zeroed input blocks
//   - mode=sine           generates a sine wave on input
//   - mode=loopback       routes written output blocks back into input after latency
//   - latency_ms=<int>    loopback latency in milliseconds (approx, block-quantized)
//   - jitter_ms=<int>     adds random extra block delay in loopback (block-quantized)
//   - eos_blocks=<int>    after N successful input reads, return EndOfStream
//   - sine_hz=<float>     sine frequency (Hz) for mode=sine
//   - amplitude=<float>   sine amplitude (0..1) for mode=sine
//
// Notes:
//   - Timing is steady_clock-based and monotonic.
//   - Latency/jitter are quantized to whole blocks to remain deterministic-ish and simple.
//   - This backend does not depend on OS audio APIs.

#include "hardware_backend.hpp"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <limits>
#include <mutex>
#include <random>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace apms::hw {

namespace {

// ---------- small helpers ----------
static inline std::string trim(std::string s) {
    auto is_ws = [](unsigned char c) { return std::isspace(c) != 0; };
    while (!s.empty() && is_ws(static_cast<unsigned char>(s.front()))) s.erase(s.begin());
    while (!s.empty() && is_ws(static_cast<unsigned char>(s.back()))) s.pop_back();
    return s;
}

static inline std::unordered_map<std::string, std::string> parse_kv_options(const std::string& opaque) {
    // Accept separators: ';' or '\n'
    // Pairs: key=value
    std::unordered_map<std::string, std::string> out;

    std::stringstream ss(opaque);
    std::string token;
    while (std::getline(ss, token, ';')) {
        token = trim(token);
        if (token.empty()) continue;

        auto eq = token.find('=');
        if (eq == std::string::npos) {
            out[trim(token)] = "1";
            continue;
        }
        auto key = trim(token.substr(0, eq));
        auto val = trim(token.substr(eq + 1));
        if (!key.empty()) out[key] = val;
    }
    return out;
}

static inline bool parse_bool(const std::string& s, bool fallback) {
    std::string v = s;
    std::transform(v.begin(), v.end(), v.begin(), [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
    if (v == "1" || v == "true" || v == "yes" || v == "on") return true;
    if (v == "0" || v == "false" || v == "no" || v == "off") return false;
    return fallback;
}

static inline int parse_int(const std::string& s, int fallback) {
    try {
        std::size_t idx = 0;
        int v = std::stoi(s, &idx);
        if (idx == 0) return fallback;
        return v;
    } catch (...) {
        return fallback;
    }
}

static inline double parse_double(const std::string& s, double fallback) {
    try {
        std::size_t idx = 0;
        double v = std::stod(s, &idx);
        if (idx == 0) return fallback;
        return v;
    } catch (...) {
        return fallback;
    }
}

static inline uint32_t blocks_for_ms(uint32_t sample_rate_hz, uint16_t frames_per_block, int latency_ms) {
    if (latency_ms <= 0) return 0;

    const double seconds = static_cast<double>(latency_ms) / 1000.0;
    const double frames  = seconds * static_cast<double>(sample_rate_hz);
    const double blocks  = frames / static_cast<double>(frames_per_block);

    // round to nearest block
    double rounded = std::floor(blocks + 0.5);
    if (rounded < 0.0) rounded = 0.0;
    if (rounded > static_cast<double>(std::numeric_limits<uint32_t>::max())) {
        return std::numeric_limits<uint32_t>::max();
    }
    return static_cast<uint32_t>(rounded);
}

} // namespace

class ReferenceBackend final : public IHardwareBackend {
public:
    ReferenceBackend() = default;
    ~ReferenceBackend() override { shutdown(); }

    std::string name() const override { return "reference"; }

    uint32_t capabilities() const noexcept override {
        return cap_mask(Capability::AudioInput) |
               cap_mask(Capability::AudioOutput) |
               cap_mask(Capability::DeviceEnumeration) |
               cap_mask(Capability::LowLatencyHint);
    }

    Status initialize(const BackendConfig& cfg) override {
        std::lock_guard<std::mutex> lk(mu_);
        if (initialized_) {
            // allow re-init by shutting down first
            shutdown_locked_();
        }

        cfg_ = cfg;

        // Basic validation
        if (cfg_.input_format.sample_rate_hz == 0 ||
            cfg_.input_format.channels == 0 ||
            cfg_.input_format.frames_per_block == 0) {
            return Status::InvalidArgument;
        }
        if (cfg_.output_format.sample_rate_hz == 0 ||
            cfg_.output_format.channels == 0 ||
            cfg_.output_format.frames_per_block == 0) {
            return Status::InvalidArgument;
        }

        // Options
        auto opt = parse_kv_options(cfg_.opaque_options);

        mode_ = Mode::Silence;
        if (auto it = opt.find("mode"); it != opt.end()) {
            std::string m = it->second;
            std::transform(m.begin(), m.end(), m.begin(),
                           [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
            if (m == "silence" || m == "zero") mode_ = Mode::Silence;
            else if (m == "sine" || m == "tone") mode_ = Mode::Sine;
            else if (m == "loopback" || m == "lb") mode_ = Mode::Loopback;
            else {
                log_(LogLevel::Warn, "reference backend: unknown mode='" + it->second + "', defaulting to silence");
                mode_ = Mode::Silence;
            }
        }

        latency_ms_ = parse_int(opt.count("latency_ms") ? opt["latency_ms"] : "", 0);
        jitter_ms_  = parse_int(opt.count("jitter_ms")  ? opt["jitter_ms"]  : "", 0);

        eos_blocks_ = parse_int(opt.count("eos_blocks") ? opt["eos_blocks"] : "", -1);

        sine_hz_    = parse_double(opt.count("sine_hz") ? opt["sine_hz"] : "", 440.0);
        amplitude_  = parse_double(opt.count("amplitude") ? opt["amplitude"] : "", 0.1);
        amplitude_  = std::clamp(amplitude_, 0.0, 1.0);

        // Quantize delay to blocks (based on INPUT format)
        base_delay_blocks_  = blocks_for_ms(cfg_.input_format.sample_rate_hz,
                                            cfg_.input_format.frames_per_block,
                                            latency_ms_);
        jitter_blocks_max_  = blocks_for_ms(cfg_.input_format.sample_rate_hz,
                                            cfg_.input_format.frames_per_block,
                                            jitter_ms_);

        // Deterministic-ish seed: stable within run but different across machines/processes
        // (If you want strict determinism, set jitter_ms=0.)
        rng_.seed(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(this)));

        // Reset state
        started_ = false;
        stopping_ = false;
        input_sequence_ = 0;
        output_sequence_ = 0;
        input_reads_ = 0;
        sine_phase_ = 0.0;

        // Clear queues
        input_q_.clear();
        delay_line_.clear();
        last_output_.reset();

        initialized_ = true;

        // Informational log
        {
            std::ostringstream oss;
            oss << "reference backend initialized: mode=" << mode_string_()
                << " in(sr=" << cfg_.input_format.sample_rate_hz
                << ", ch=" << cfg_.input_format.channels
                << ", fpb=" << cfg_.input_format.frames_per_block << ")"
                << " out(sr=" << cfg_.output_format.sample_rate_hz
                << ", ch=" << cfg_.output_format.channels
                << ", fpb=" << cfg_.output_format.frames_per_block << ")"
                << " delay_blocks=" << base_delay_blocks_
                << " jitter_blocks_max=" << jitter_blocks_max_;
            log_(LogLevel::Info, oss.str());
        }

        return Status::Ok;
    }

    Status start() override {
        std::lock_guard<std::mutex> lk(mu_);
        if (!initialized_) return Status::NotReady;
        if (started_) return Status::Ok;
        stopping_ = false;
        started_ = true;
        cv_.notify_all();
        return Status::Ok;
    }

    Status stop() override {
        std::lock_guard<std::mutex> lk(mu_);
        if (!initialized_) return Status::NotReady;
        if (!started_) return Status::Ok;
        stopping_ = true;
        started_ = false;
        cv_.notify_all();
        return Status::Ok;
    }

    void shutdown() override {
        std::lock_guard<std::mutex> lk(mu_);
        shutdown_locked_();
    }

    Status read_input_block(AudioBlock& out,
                            std::optional<Nanoseconds> timeout = std::nullopt) override {
        // fast check: EOS
        if (eos_blocks_ >= 0) {
            // eos_blocks_ counts successful reads
            if (input_reads_.load(std::memory_order_relaxed) >= static_cast<uint64_t>(eos_blocks_)) {
                return Status::EndOfStream;
            }
        }

        // Ensure backend started
        if (!wait_started_(timeout)) return Status::Timeout;
        if (!initialized_.load(std::memory_order_relaxed)) return Status::NotReady;

        // Prepare block
        out.format = cfg_.input_format;
        out.resize_for_format();

        const auto ts = now();

        // Produce according to mode
        if (mode_ == Mode::Loopback) {
            Status s = read_loopback_(out, timeout);
            if (s != Status::Ok) return s;
            // Timestamp will reflect when block becomes available
            if (out.timestamp == TimePoint{}) out.timestamp = ts;
        } else if (mode_ == Mode::Sine) {
            generate_sine_(out);
            out.timestamp = ts;
            out.sequence = next_input_seq_();
        } else { // Silence
            std::fill(out.samples.begin(), out.samples.end(), 0.0f);
            out.timestamp = ts;
            out.sequence = next_input_seq_();
        }

        input_reads_.fetch_add(1, std::memory_order_relaxed);
        return Status::Ok;
    }

    Status write_output_block(const AudioBlock& in,
                              std::optional<Nanoseconds> timeout = std::nullopt) override {
        (void)timeout;

        if (!wait_started_(timeout)) return Status::Timeout;
        if (!initialized_.load(std::memory_order_relaxed)) return Status::NotReady;

        // Validate shape for output format
        if (in.format.sample_rate_hz != cfg_.output_format.sample_rate_hz ||
            in.format.channels != cfg_.output_format.channels ||
            in.format.frames_per_block != cfg_.output_format.frames_per_block) {
            return Status::InvalidArgument;
        }
        if (!in.valid_shape()) return Status::InvalidArgument;

        // Store output as "last output"
        {
            std::lock_guard<std::mutex> lk(mu_);
            last_output_ = in;
            // If caller didn't set timestamp, set "now" (helpful for loopback)
            if (last_output_->timestamp == TimePoint{}) {
                last_output_->timestamp = now();
            }
            last_output_->sequence = next_output_seq_();

            if (mode_ == Mode::Loopback) {
                // Convert output format -> input format if they differ.
                // For reference backend: we only support exact format parity for loopback.
                if (cfg_.output_format.sample_rate_hz != cfg_.input_format.sample_rate_hz ||
                    cfg_.output_format.channels != cfg_.input_format.channels ||
                    cfg_.output_format.frames_per_block != cfg_.input_format.frames_per_block) {
                    return Status::Unsupported;
                }
                enqueue_loopback_locked_(*last_output_);
            }
        }

        return Status::Ok;
    }

    std::vector<DeviceInfo> list_input_devices() override {
        // Reference backend exposes "virtual" devices for clarity.
        return {
            {"ref:input0",  "Reference Virtual Input",  true},
            {"ref:input1",  "Reference Virtual Input (alt)", false}
        };
    }

    std::vector<DeviceInfo> list_output_devices() override {
        return {
            {"ref:output0", "Reference Virtual Output", true},
            {"ref:output1", "Reference Virtual Output (alt)", false}
        };
    }

private:
    enum class Mode { Silence, Sine, Loopback };

    std::string mode_string_() const {
        switch (mode_) {
            case Mode::Silence:  return "silence";
            case Mode::Sine:     return "sine";
            case Mode::Loopback: return "loopback";
            default:             return "unknown";
        }
    }

    bool wait_started_(std::optional<Nanoseconds> timeout) {
        std::unique_lock<std::mutex> lk(mu_);

        if (!initialized_) return false;

        if (started_) return true;

        if (!timeout.has_value()) {
            cv_.wait(lk, [&] { return started_ || stopping_ || !initialized_; });
            return started_ && initialized_;
        }

        auto ok = cv_.wait_for(lk, *timeout, [&] { return started_ || stopping_ || !initialized_; });
        if (!ok) return false;
        return started_ && initialized_;
    }

    uint64_t next_input_seq_() {
        return input_sequence_.fetch_add(1, std::memory_order_relaxed) + 1;
    }

    uint64_t next_output_seq_() {
        return output_sequence_.fetch_add(1, std::memory_order_relaxed) + 1;
    }

    void shutdown_locked_() {
        if (!initialized_) return;

        stopping_ = true;
        started_ = false;

        input_q_.clear();
        delay_line_.clear();
        last_output_.reset();

        initialized_ = false;
        cv_.notify_all();
    }

    // -------- loopback internals --------
    void enqueue_loopback_locked_(const AudioBlock& blk) {
        // Delay line holds blocks that will later become available as input.
        // We implement delay as "N empty slots" then block enters the line and pops out.
        // Add optional jitter by pushing extra empty slots.
        uint32_t extra = 0;
        if (jitter_blocks_max_ > 0) {
            std::uniform_int_distribution<uint32_t> dist(0, jitter_blocks_max_);
            extra = dist(rng_);
        }

        // Ensure base delay empties exist only when needed
        // We represent "empty" as std::nullopt in the delay line.
        for (uint32_t i = 0; i < base_delay_blocks_ + extra; ++i) {
            delay_line_.push_back(std::nullopt);
        }

        delay_line_.push_back(blk);

        // Drain delay line into input queue for any ready blocks
        drain_delay_line_locked_();
        cv_.notify_all();
    }

    void drain_delay_line_locked_() {
        // Any leading "ready" blocks (i.e., non-nullopt) can be moved,
        // but we must also pop through null slots to simulate time passing.
        // Here we pop at most one slot per output write to avoid collapsing delay.
        // However, for simplicity, we will pop and move blocks that are at the front
        // only when the front is a real block.
        while (!delay_line_.empty() && delay_line_.front().has_value()) {
            input_q_.push_back(std::move(*delay_line_.front()));
            delay_line_.pop_front();
        }

        // Pop a single null slot per write to simulate time advancing in the delay pipeline.
        if (!delay_line_.empty() && !delay_line_.front().has_value()) {
            delay_line_.pop_front();
        }

        // After popping a null, we might have exposed a block.
        while (!delay_line_.empty() && delay_line_.front().has_value()) {
            input_q_.push_back(std::move(*delay_line_.front()));
            delay_line_.pop_front();
        }
    }

    Status read_loopback_(AudioBlock& out, std::optional<Nanoseconds> timeout) {
        std::unique_lock<std::mutex> lk(mu_);

        // Wait for a block to exist in input queue
        auto has_block = [&] { return !input_q_.empty() || stopping_ || !started_ || !initialized_; };

        if (!has_block()) {
            if (!timeout.has_value()) {
                cv_.wait(lk, has_block);
            } else {
                if (!cv_.wait_for(lk, *timeout, has_block)) {
                    return Status::Timeout;
                }
            }
        }

        if (!initialized_) return Status::NotReady;
        if (!started_) return Status::NotReady;
        if (stopping_) return Status::NotReady;

        if (input_q_.empty()) {
            // No data available
            return Status::NotReady;
        }

        AudioBlock blk = std::move(input_q_.front());
        input_q_.pop_front();

        // Copy into out (formats should already match)
        out.format = blk.format;
        out.samples = std::move(blk.samples);
        out.timestamp = blk.timestamp;
        out.sequence = next_input_seq_();

        // Ensure buffer shape is correct even if upstream gave weird size
        if (!out.valid_shape()) {
            out.resize_for_format();
        }

        return Status::Ok;
    }

    // -------- sine generator --------
    void generate_sine_(AudioBlock& out) {
        // Generate per-sample sine across all channels (same tone each channel).
        // Determinism: purely phase-accumulated by block.
        const double sr   = static_cast<double>(out.format.sample_rate_hz);
        const double w    = 2.0 * 3.14159265358979323846 * sine_hz_ / sr;
        const size_t ch   = static_cast<size_t>(out.format.channels);
        const size_t fpb  = static_cast<size_t>(out.format.frames_per_block);

        for (size_t f = 0; f < fpb; ++f) {
            const float v = static_cast<float>(amplitude_ * std::sin(sine_phase_));
            sine_phase_ += w;
            if (sine_phase_ > 2.0 * 3.14159265358979323846) {
                sine_phase_ -= 2.0 * 3.14159265358979323846;
            }

            const size_t base = f * ch;
            for (size_t c = 0; c < ch; ++c) {
                out.samples[base + c] = v;
            }
        }
    }

private:
    // Config/state
    BackendConfig cfg_{};

    std::mutex mu_;
    std::condition_variable cv_;

    std::atomic<bool> initialized_{false};
    bool started_ = false;
    bool stopping_ = false;

    Mode mode_ = Mode::Silence;

    int latency_ms_ = 0;
    int jitter_ms_  = 0;

    uint32_t base_delay_blocks_ = 0;
    uint32_t jitter_blocks_max_ = 0;

    int eos_blocks_ = -1; // -1 disables EOS

    double sine_hz_   = 440.0;
    double amplitude_ = 0.1;
    double sine_phase_ = 0.0;

    std::atomic<uint64_t> input_sequence_{0};
    std::atomic<uint64_t> output_sequence_{0};
    std::atomic<uint64_t> input_reads_{0};

    // Loopback queues
    std::deque<AudioBlock> input_q_;
    std::deque<std::optional<AudioBlock>> delay_line_;
    std::optional<AudioBlock> last_output_;

    // RNG (only used for jitter)
    std::mt19937 rng_;
};

// ---- Factory registration (optional) ----
// If you prefer explicit wiring, you can remove this and register from your app.
static BackendAutoRegister kRefBackendReg{
    "reference",
    []() { return std::make_unique<ReferenceBackend>(); }
};

} // namespace apms::hw
