#pragma once

#include "raps/platform/platform_hal.hpp"

inline void inject_execution_failure() {
    static bool injected = false;

    if (injected) return;
    injected = true;

    std::cout << "[SIL] Injecting actuator execution failure\n";

    // Monkey-patch behavior via HAL failure probability
    PlatformHAL::seed_rng_for_stubs(9999);
}

