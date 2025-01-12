#include "inc.hh"
namespace {

global usize g_memory_page_size = {0};

MemoryReservation memory_reserve() {
    void* ptr = mmap(NULL, MEMORY_RESERVE_SIZE, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (ptr == MAP_FAILED) Panic("memory_reserve mmap failed");

    return {
        .base             = (u8*)ptr,
        .block_size       = g_memory_page_size,
        .blocks_committed = 0,
    };
}

void memory_commit_size(MemoryReservation* reservation, usize total_size) {
    usize total_blocks_required = 1 + total_size / reservation->block_size;

    if (total_blocks_required > reservation->blocks_committed) {
        usize cur_size                = reservation->block_size * reservation->blocks_committed;
        usize add_size                = total_blocks_required - reservation->blocks_committed;
        reservation->blocks_committed = total_blocks_required;

        i32 result = mprotect(
            reservation->base + cur_size, add_size * reservation->block_size, PROT_READ | PROT_WRITE
        );
        if (result == -1) Panic("memory_commit_size mprotect failed");

    } else if (total_blocks_required < reservation->blocks_committed) {
        usize remove_size             = reservation->blocks_committed - total_blocks_required;
        reservation->blocks_committed = total_blocks_required;
        usize new_size                = reservation->block_size * reservation->blocks_committed;

        i32 result = madvise(
            reservation->base + new_size, remove_size * reservation->block_size, MADV_DONTNEED
        );
        if (result == -1) Panic("memory_commit_size madvise failed");
    }
}

void memory_release(MemoryReservation* reservation) {
    i32 result = munmap(reservation->base, MEMORY_RESERVE_SIZE);
    if (result == -1) Panic("memory_decommit munmap failed");
    ZeroStruct(reservation);
}

void* memory_heap_alloc(usize size) {
    return calloc(1, size);
}

void memory_heap_free(void* ptr) {
    free(ptr);
}

MemoryAllocator memory_get_global_allocator() {
    if (!g_memory_page_size) {
        g_memory_page_size = sysconf(_SC_PAGESIZE);
    }
    return {
        .memory_reserve     = memory_reserve,
        .memory_commit_size = memory_commit_size,
        .memory_release     = memory_release,
        .memory_heap_alloc  = memory_heap_alloc,
        .memory_heap_free   = memory_heap_free,
    };
}

}