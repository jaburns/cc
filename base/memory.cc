#include "inc.hh"
namespace a {
// -----------------------------------------------------------------------------

global usize g_memory_page_size = {0};
global thread_local MemoryAllocator g_memory_allocator;

// -----------------------------------------------------------------------------

MemoryReservation memory_reservation_acquire(usize reserve_size) {
    if (reserve_size == 0) {
        reserve_size = MEMORY_RESERVE_DEFAULT_SIZE;
    } else {
        usize pages = (reserve_size % g_memory_page_size == 0 ? 0 : 1) + reserve_size / g_memory_page_size;
        reserve_size = g_memory_page_size * pages;
    }

    void* ptr = mmap(NULL, reserve_size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    AssertM(ptr != MAP_FAILED, "memory_reservation_acquire mmap failed");

    return {
        .base = (u8*)ptr,
        .pages_committed = 0,
        .reserved_size = reserve_size,
    };
}

void memory_reservation_commit(MemoryReservation* reservation, usize total_commit_size) {
    usize total_pages_required = 1 + total_commit_size / g_memory_page_size;

    if (total_pages_required > reservation->pages_committed) {
        AssertM(total_pages_required * g_memory_page_size <= reservation->reserved_size, "memory_reservation_commit would overrun the end of the reserved address space");

        usize cur_size = g_memory_page_size * reservation->pages_committed;
        usize add_pages = total_pages_required - reservation->pages_committed;
        reservation->pages_committed = total_pages_required;

        int result = mprotect(
            reservation->base + cur_size, add_pages * g_memory_page_size, PROT_READ | PROT_WRITE
        );
        AssertM(result != -1, "memory_reservation_commit mprotect failed");

    } else if (total_pages_required < reservation->pages_committed) {
        usize remove_size = reservation->pages_committed - total_pages_required;
        reservation->pages_committed = total_pages_required;
        usize new_size = g_memory_page_size * reservation->pages_committed;

        int result = madvise(
            reservation->base + new_size, remove_size * g_memory_page_size, MADV_DONTNEED
        );
        AssertM(result != -1, "memory_reservation_commit madvise failed");
    }
}

void memory_reservation_trim(MemoryReservation* reservation) {
    usize keep_size = reservation->pages_committed * g_memory_page_size;
    if (keep_size == 0 || keep_size == reservation->reserved_size) return;

    usize trim_size = reservation->reserved_size - keep_size;
    u8* trim_addr = reservation->base + keep_size;
    int result = munmap(trim_addr, trim_size);
    AssertM(result != -1, "memory_reservation_trim munmap failed");

    reservation->reserved_size = keep_size;
}

void memory_reservation_release(MemoryReservation* reservation) {
    int result = munmap(reservation->base, reservation->reserved_size);
    AssertM(result != -1, "memory_reservation_release munmap failed");
    StructZero(reservation);
}

// -----------------------------------------------------------------------------

MemoryAllocator memory_allocator_create() {
    if (!g_memory_page_size) {
        g_memory_page_size = sysconf(_SC_PAGESIZE);
    }
    return {
        .memory_reservation_acquire = memory_reservation_acquire,
        .memory_reservation_commit = memory_reservation_commit,
        .memory_reservation_trim = memory_reservation_trim,
        .memory_reservation_release = memory_reservation_release,
    };
}

void memory_set_global_allocator(MemoryAllocator allocator) {
    g_memory_allocator = allocator;
}

MemoryAllocator memory_get_global_allocator() {
    return g_memory_allocator;
}

// -----------------------------------------------------------------------------
}  // namespace a
