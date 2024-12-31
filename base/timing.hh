#pragma once
#include "inc.hh"
namespace {

void timing_global_init();
u64  timing_get_ticks();
u64  timing_ticks_to_nanos(u64 ticks);

}