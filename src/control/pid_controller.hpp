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
    float elapsed_ms) {

    // Integral term with anti-windup
    float dt_s = 0.0f;
    if (elapsed_ms > 0.0f) {
        dt_s = elapsed_ms / 1000.0f;
    }

    if (dt_s > 0.0f) {
        integral += error * dt_s;
        integral = std::max(
            -integral_limit,
            std::min(integral_limit, integral)
        );
    }

    float derivative = 0.0f;
    if (dt_s > 0.0f) {
        derivative = (error - previous_error) / dt_s;
    }

    float output =
        (kp * error) +
        (ki * integral) +
        (kd * derivative);

    previous_error = error;
    return output;
}
