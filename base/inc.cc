#include "inc.hh"

#include "timing.cc"
#include "memory.cc"
#include "array.cc"
#include "arena.cc"
#include "string.cc"
#include "math.cc"
#include "hasharray.cc"
#include "channel.cc"
#include "fs.cc"
#include "json.cc"
#include "test.cc"

namespace a {
void base_global_init() {
    timing_global_init();
    memory_set_global_allocator(memory_allocator_create());
    local_persist thread_local Arena scratch[2] = {};
    scratch[0].create(1_gb);
    scratch[1].create(1_gb);
    arena_bind_global_scratch(&scratch[0], &scratch[1]);
}
}  // namespace a
