#pragma once

namespace raps::platform {

// Stub mutex for host/demo builds.
// Replace internals with your RTOS primitive later.
class RtosMutex {
public:
    void lock()   {}
    void unlock() {}
};

class LockGuard {
public:
    explicit LockGuard(RtosMutex& m) : m_(m) { m_.lock(); }
    ~LockGuard() { m_.unlock(); }
    LockGuard(const LockGuard&) = delete;
    LockGuard& operator=(const LockGuard&) = delete;
private:
    RtosMutex& m_;
};

} // namespace raps::platform
