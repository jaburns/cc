#include "inc.hh"
namespace a {
// -----------------------------------------------------------------------------

global thread_local Arena* g_arena_scratch[2];

// -----------------------------------------------------------------------------

void Arena::create(usize reserve_size) {
    DebugAssert(!cur);
    reservation = memory_get_global_allocator().memory_reservation_acquire(reserve_size);
    cur = reservation.base;
    using_reservation = true;
}

Arena Arena::make_with_buffer(u8* bytes, usize count) {
    Arena ret = {};

    ret.reservation.base = bytes;
    ret.reservation.reserved_size = count;
    ret.cur = bytes;
    ret.using_reservation = false;

    return ret;
}

void Arena::destroy() {
    DebugAssert(cur);
    clear();
    memory_get_global_allocator().memory_reservation_release(&reservation);
    StructZero(this);
}

void Arena::release_unused_pages() {
    DebugAssert(cur);
    memory_get_global_allocator().memory_reservation_commit(&reservation, cur - reservation.base);
}

void Arena::align(usize alignment) {
    DebugAssert(cur);
    cur = (u8*)(((usize)cur + (alignment - 1)) & ~(alignment - 1));
}

forall(T) void Arena::align() {
    align(sizeof(T));
}

// TODO(jaburns) arena shouldn't do a virtual call if it doesn't need more pages.
// probably can roll MemoryAllocator right into Arena since it's the only thing
// that uses it, though we need to set the global function pointers still from
// outside the dll.

forall(T) T* Arena::push() {
    DebugAssert(cur);

    usize size = sizeof(T);

    cur = (u8*)(((usize)cur + (alignof(T) - 1)) & ~(alignof(T) - 1));

    u8* ret = cur;
    cur += size;
    if (using_reservation) {
        memory_get_global_allocator().memory_reservation_commit(&reservation, cur - reservation.base);
    } else if (cur - reservation.base >= reservation.reserved_size) {
        Panic("arena overran backing buffer");
    }

    return (T*)memset(ret, 0, size);
}

forall(T) Slice<T> Arena::push_many(usize count) {
    DebugAssert(cur);

    if (count == 0) return Slice<T>{};

    usize size = sizeof(T) * count;

    cur = (u8*)(((usize)cur + (alignof(T) - 1)) & ~(alignof(T) - 1));

    u8* ret = cur;
    cur += size;
    if (using_reservation) {
        memory_get_global_allocator().memory_reservation_commit(&reservation, cur - reservation.base);
    } else if (cur - reservation.base >= reservation.reserved_size) {
        Panic("arena overran backing buffer");
    }

    return Slice<T>{(T*)memset(ret, 0, size), count};
}

void* Arena::push_mem(void* start, usize size) {
    DebugAssert(cur);
    if (size == 0) return nullptr;

    u8* ret = cur;
    cur += size;
    if (using_reservation) {
        memory_get_global_allocator().memory_reservation_commit(&reservation, cur - reservation.base);
    } else if (cur - reservation.base >= reservation.reserved_size) {
        Panic("arena overran backing buffer");
    }

    return MemCopy(ret, start, size);
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
