#pragma once

#include <algorithm>

// Generic PID controller core
inline float compute_pid_output(
    float error,
    float& integral,
    float& previous_error,
    float kp,
    float ki,
    float kd,
    float integral_limit,
    uint64_t elapsed_ms) {

    if (elapsed_ms == 0) {
        // Zero-time tick: return proportional-only to avoid division by zero
        return kp * error;
    }
    float dt_s = static_cast<float>(elapsed_ms) / 1000.0f;

    integral += error * dt_s;
    integral = std::max(
        -integral_limit,
        std::min(integral_limit, integral)
    );

    float derivative = (error - previous_error) / dt_s;

    float output =
        (kp * error) +
        (ki * integral) +
        (kd * derivative);

    previous_error = error;
    return output;
}
