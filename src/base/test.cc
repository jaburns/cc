#include "inc.hh"
namespace {

void x_test_run(cchar* name, void (*fn)(void)) {
    timing_global_init();
    fprintf(stderr, "> Running: %s", name);
    u64 start_ticks = timing_get_ticks();
    fn();
    u64 finish_ticks = timing_get_ticks();
    fprintf(stderr, "\t- completed in %llu μs\n", timing_ticks_to_nanos(finish_ticks - start_ticks) / 1000);
}

#if TEST
void test_base() {
    test_run(test_channel);
}
#endif

}  // namespace