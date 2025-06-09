#include "inc.hh"
namespace a {
// -----------------------------------------------------------------------------

#define MEMORY_RESERVE_DEFAULT_SIZE 1_mb

global thread_local Arena* g_arena_scratch[2];
u32 Arena::log_reserve_page_size;
u32 Arena::log_commit_page_size;

// -----------------------------------------------------------------------------

void Arena::global_init() {
    u32 page_size = (u32)sysconf(_SC_PAGESIZE);
    u32 log_page_size = (8 /*bits*/ * sizeof(u32) - count_leading_zeroes(page_size)) - 1;
    AssertM(1 << log_page_size == page_size, "Arena::global_init failed: unexpected non-power-of-two page size");

    Arena::log_reserve_page_size = log_page_size;
    Arena::log_commit_page_size = log_page_size;
}

void Arena::thread_init(Arena* scratch0, Arena* scratch1) {
    g_arena_scratch[0] = scratch0;
    g_arena_scratch[1] = scratch1;
}

// -----------------------------------------------------------------------------

void Arena::create(usize reserve_size) {
    DebugAssert(!cur);

    if (reserve_size == 0) {
        reserve_size = MEMORY_RESERVE_DEFAULT_SIZE;
    } else {
        usize mem_reserve_size = 1 << log_reserve_page_size;
        usize pages = (reserve_size % mem_reserve_size == 0 ? 0 : 1) + reserve_size / mem_reserve_size;
        reserve_size = mem_reserve_size * pages;
    }

    u8* ptr = mem_reserve(reserve_size);

    base = ptr,
    pages_committed = 0,
    reserved_size = reserve_size,
    cur = base;
}

Arena Arena::make_with_buffer(u8* bytes, usize count) {
    Arena ret = {};
    ret.base = bytes;
    ret.reserved_size = count;
    ret.pages_committed = USIZE_MAX;
    ret.cur = bytes;
    return ret;
}

void Arena::destroy() {
    if (cur && pages_committed != USIZE_MAX) {
        mem_release(base, reserved_size);
    }
    ZeroStruct(this);
}

void Arena::max_align() {
    DebugAssert(cur);
    konst usize MAX_ALIGN = 32;
    cur = (u8*)(((usize)cur + (MAX_ALIGN - 1)) & ~(MAX_ALIGN - 1));
}

forall(T) void Arena::align() {
    DebugAssert(cur);
    cur = (u8*)(((usize)cur + (alignof(T) - 1)) & ~(alignof(T) - 1));
}

forall(T) T* Arena::push() {
    DebugAssert(cur);
    if constexpr (sizeof(T) > 1) {
        cur = (u8*)(((usize)cur + (alignof(T) - 1)) & ~(alignof(T) - 1));
    }
    u8* ret = cur;
    bump(sizeof(T));
    return (T*)memset(ret, 0, sizeof(T));
}

forall(T) T* Arena::push_unaligned() {
    DebugAssert(cur);
    u8* ret = cur;
    bump(sizeof(T));
    return (T*)memset(ret, 0, sizeof(T));
}

forall(T) Slice<T> Arena::push_many(usize count) {
    DebugAssert(cur);
    if (count == 0) return Slice<T>{};
    if constexpr (sizeof(T) > 1) {
        cur = (u8*)(((usize)cur + (alignof(T) - 1)) & ~(alignof(T) - 1));
    }
    u8* ret = cur;
    usize size = sizeof(T) * count;
    bump(size);
    return Slice<T>{(T*)memset(ret, 0, size), count};
}

forall(T) Slice<T> Arena::push_many_unaligned(usize count) {
    DebugAssert(cur);
    if (count == 0) return Slice<T>{};
    u8* ret = cur;
    usize size = sizeof(T) * count;
    bump(size);
    return Slice<T>{(T*)memset(ret, 0, size), count};
}

void Arena::push_bytes(void* start, usize size) {
    DebugAssert(cur);
    if (size == 0) return;
    u8* ret = cur;
    bump(size);
    MemCopy(ret, start, size);
}

void Arena::bump(usize size) {
    cur += size;

    if (pages_committed == USIZE_MAX) {
        AssertM(cur - base < reserved_size, "arena overran backing buffer");
        return;
    }

    usize total_commit_size = (usize)(cur - base);
    usize total_pages_required = 1 + ((total_commit_size - 1) >> log_commit_page_size);

    if (total_pages_required > pages_committed) {
        AssertM(total_pages_required << log_commit_page_size <= reserved_size, "memory_reservation_commit would overrun the end of the reserved address space");

        usize cur_size = pages_committed << log_commit_page_size;
        usize add_pages = total_pages_required - pages_committed;
        pages_committed = total_pages_required;

        mem_commit(base + cur_size, add_pages << log_commit_page_size);
    }
}

u8* Arena::mem_reserve(usize size) {
    return (u8*)mmap(NULL, size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
}

void Arena::mem_commit(u8* ptr, usize size) {
    int result = mprotect(ptr, size, PROT_READ | PROT_WRITE);
    AssertM(result != -1, "Arena::mem_commit mprotect failed");
}

void Arena::mem_release(u8* ptr, usize size) {
    int result = munmap(ptr, size);
    AssertM(result != -1, "Arena::mem_release munmup failed");
}

// -----------------------------------------------------------------------------

void arena_bind_global_scratch(Arena* scratch0, Arena* scratch1) {
    g_arena_scratch[0] = scratch0;
    g_arena_scratch[1] = scratch1;
}

ScratchArena::ScratchArena() {
    arena = g_arena_scratch[0];
    arena_mark = arena->cur;
}

ScratchArena::ScratchArena(Arena* conflict) {
    Arena* conflicts[] = {conflict};
    init(SliceFromRawArray(Arena*, conflicts));
}

ScratchArena::ScratchArena(Slice<Arena*> conflicts) {
    init(conflicts);
}

ScratchArena::~ScratchArena() {
    arena->cur = arena_mark;
}

void ScratchArena::init(Slice<Arena*> conflicts) {
    bool matched[2] = {};
    for (u32 i = 0; i < conflicts.count; ++i) {
        if (g_arena_scratch[0] == conflicts.elems[i]) {
            matched[0] = true;
            if (matched[1]) goto err;
        } else if (g_arena_scratch[1] == conflicts.elems[i]) {
            matched[1] = true;
            if (matched[0]) goto err;
        }
    }

    arena = g_arena_scratch[matched[0] ? 1 : 0];
    arena_mark = arena->cur;

    return;
err:
    Panic("both scratch arenas were passed as conflicts to ScratchArena::init");
}

// -----------------------------------------------------------------------------
}  // namespace a
