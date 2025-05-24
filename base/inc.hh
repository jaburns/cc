#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <atomic>

#include <sys/mman.h>
#include <sys/stat.h>
#include <pthread.h>
#include <unistd.h>
#include <dirent.h>

#ifdef __APPLE__
#include <mach/mach_time.h>
#else
#include <time.h>
#endif

#include "../../vendor/simde/arm/neon.h"

#include "defs.hh"
#include "timing.hh"
#include "simd.hh"
#include "array.hh"
#include "arena.hh"
#include "string.hh"

struct StructMemberInfo {
    a::Str type;
    a::Str name;
    usize size;
    usize offset;
};

#include "math.hh"
#include "hash.hh"
#include "hasharray.hh"
#include "channel.hh"
#include "fs.hh"
#include "json.hh"
#include "bindump.hh"
#include "test.hh"
