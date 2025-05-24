#include "inc.hh"

#include "timing.cc"
#include "array.cc"
#include "arena.cc"
#include "string.cc"
#include "math.cc"
#include "hasharray.cc"
#include "channel.cc"
#include "fs.cc"
#include "json.cc"
#include "bindump.cc"
#include "test.cc"

namespace a {
void base_global_init() {
    timing_global_init();
    Arena::global_init();
    local_persist thread_local Arena scratch[2] = {};
    scratch[0].create(1_gb);
    scratch[1].create(1_gb);
    Arena::thread_init(&scratch[0], &scratch[1]);
}
}  // namespace a
