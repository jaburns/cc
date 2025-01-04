#pragma once
#include "inc.hh"
namespace {

#define MEMORY_RESERVE_SIZE       32_gb
#define MEMORY_DEFAULT_BLOCK_SIZE 1_mb

struct MemoryReservation {
    u8*   base;
    usize block_size;
    usize blocks_committed;
};

struct MemoryAllocator {
    MemoryReservation (*memory_reserve)(usize block_size);
    void              (*memory_commit_size)(MemoryReservation* reservation, usize total_size);
    void              (*memory_release)(MemoryReservation* reservation);
    void*             (*memory_heap_alloc)(usize size);
    void              (*memory_heap_free)(void* ptr);
};

MemoryAllocator memory_get_global_allocator();

}