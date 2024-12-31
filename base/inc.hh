#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

#include <atomic>

#include <sys/mman.h>
#include <pthread.h>
#include <unistd.h>

#ifdef __APPLE__
#include <mach/mach_time.h>
#else
#include <time.h>
#endif

#include "../vendor/simde/arm/neon.h"

#include "defs.hh"
#include "timing.hh"
#include "simd.hh"
#include "array.hh"
#include "memory.hh"
#include "arena.hh"
#include "string.hh"
#include "math.hh"
#include "hasharray.hh"
#include "channel.hh"
#include "xml.hh"
#include "test.hh"