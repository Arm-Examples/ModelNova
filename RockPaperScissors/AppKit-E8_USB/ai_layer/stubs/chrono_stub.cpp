/**
 * @file chrono_stub.cpp
 * @brief Stub implementations for std::chrono functions
 * 
 * Provides implementations of chrono functions required by ExecuTorch
 * in bare-metal AC6 environment.
 */

#include <cstdint>

namespace std {
namespace chrono {
namespace _V2 {

// Define the time_point and duration types
typedef int64_t rep;

struct duration {
    rep count_;
    duration() : count_(0) {}
    explicit duration(rep c) : count_(c) {}
    rep count() const { return count_; }
};

struct time_point {
    duration d_;
    time_point() : d_(0) {}
    explicit time_point(const duration& dur) : d_(dur) {}
    duration time_since_epoch() const { return d_; }
};

// steady_clock implementation
class steady_clock {
public:
    typedef duration duration;
    typedef time_point time_point;
    
    static time_point now() noexcept {
        // Return epoch for bare-metal (no real clock available)
        return time_point(duration(0));
    }
};

// system_clock implementation  
class system_clock {
public:
    typedef duration duration;
    typedef time_point time_point;
    
    static time_point now() noexcept {
        return time_point(duration(0));
    }
};

} // namespace _V2

// Expose to std::chrono namespace
using steady_clock = _V2::steady_clock;
using system_clock = _V2::system_clock;

} // namespace chrono
} // namespace std

// Provide _impure_ptr for newlib/POSIX compatibility
extern "C" {

struct _reent {
    int _errno;
};

struct _reent *_impure_ptr = nullptr;

// Define timespec for POSIX compatibility
struct timespec {
    long tv_sec;   // seconds
    long tv_nsec;  // nanoseconds
};

// High resolution clock stub
typedef long long hrtime_t;
__attribute__((weak))
hrtime_t gethrtime(void) {
    return 0;
}

// clock_gettime stub
__attribute__((weak))
int clock_gettime(int clock_id, struct timespec *tp) {
    if (tp) {
        tp->tv_sec = 0;
        tp->tv_nsec = 0;
    }
    return 0;
}

}