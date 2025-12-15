#include "raps/hil/hil_device_interface.hpp"

#include <atomic>

namespace {
std::atomic<HilDeviceInterface*> g_dev{nullptr};
}

void hil_set_device(HilDeviceInterface* dev) {
    g_dev.store(dev, std::memory_order_release);
}

HilDeviceInterface* hil_get_device() {
    return g_dev.load(std::memory_order_acquire);
}
