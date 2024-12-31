#include "inc.hh"
namespace {

#ifdef __APPLE__

global mach_timebase_info_data_t g_timing_timebase;

void timing_global_init() {
    mach_timebase_info(&g_timing_timebase);
}

u64 timing_get_ticks() {
#ifdef __aarch64__
    u64 ret;
    __asm__ volatile("mrs %0, cntvct_el0" : "=r"(ret));
    return ret;
#else
    return mach_absolute_time();
#endif
}

u64 timing_ticks_to_nanos(u64 ticks) {
    return ticks * g_timing_timebase.numer / g_timing_timebase.denom;
}

#else

global u64 g_timing_start_secs;

void timing_global_init() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    g_timing_start_secs = ts.tv_sec;
}

u64 timing_get_ticks() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return 1000000000l * (ts.tv_sec - g_timing_start_secs) + ts.tv_nsec;
}

u64 timing_ticks_to_nanos(u64 ticks) {
    return ticks;
}

#endif

}  // namespace