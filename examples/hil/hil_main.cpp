#include "raps/hil/hil_device_interface.hpp"
#include "raps/hil/hil_tcp_device.hpp"
#include "platform/platform_hal.hpp"

#include <chrono>
#include <iostream>
#include <thread>

int main() {
    // 1) Create TCP device + connect to rig
    HilTcpDevice dev; // uses RAPS_HIL_TCP_HOST / PORT defaults
    dev.set_io_timeout_ms(250);

    if (!dev.connect()) {
        std::cerr << "HIL TCP connect failed. Is your rig/server running?\n";
        return 2;
    }

    // 2) Inject into PlatformHAL(HIL)
    hil_set_device(&dev);

    // 3) Smoke test
    const char* tx = "tx_hil_demo_001";
    bool ok = PlatformHAL::actuator_execute(tx, 98.0f, -0.02f, 120);
    std::cout << "actuator_execute: " << (ok ? "OK" : "FAIL") << "\n";

    // 4) Timing harness
    std::cout << "Running HIL timing harness at 50Hz for 2 seconds...\n";
    const int period_ms = 20;

    uint32_t start = PlatformHAL::now_ms();
    while (PlatformHAL::now_ms() - start < 2000u) {
        uint32_t t0 = PlatformHAL::now_ms();

        // TODO: call your supervisor/controller cycle here.

        uint32_t elapsed = PlatformHAL::now_ms() - t0;
        if (elapsed > static_cast<uint32_t>(period_ms)) {
            PlatformHAL::metric_emit("hil.deadline_miss", 1.0f);
        }

        int sleep_ms = period_ms - static_cast<int>(elapsed);
        if (sleep_ms < 0) sleep_ms = 0;
        std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));
    }

    dev.disconnect();
    std::cout << "HIL harness complete.\n";
    return 0;
}
