#pragma once
#include "inc.hh"
namespace a {
// -----------------------------------------------------------------------------

#define MEMORY_RESERVE_DEFAULT_SIZE 1_mb

struct MemoryReservation {
    u8* base;
    usize pages_committed;
    usize reserved_size;
};

struct MemoryAllocator {
    MemoryReservation (*memory_reservation_acquire)(usize reserve_size);
    void (*memory_reservation_commit)(MemoryReservation* reservation, usize total_commit_size);
    void (*memory_reservation_trim)(MemoryReservation* reservation);
    void (*memory_reservation_release)(MemoryReservation* reservation);
};

MemoryAllocator memory_allocator_create();
void memory_set_global_allocator(MemoryAllocator allocator);
MemoryAllocator memory_get_global_allocator();

// -----------------------------------------------------------------------------
}  // namespace a
