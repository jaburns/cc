#include "inc.hh"
namespace {
// -----------------------------------------------------------------------------

global thread_local Arena g_arena_scratch[2];

// -----------------------------------------------------------------------------

Arena Arena::create(MemoryAllocator allocator) {
    MemoryReservation reservation = allocator.memory_reserve();
    Arena             ret         = {};

    ret.allocator       = allocator;
    ret.reservation     = reservation;
    ret.cur             = reservation.base;
    ret.resources_stack = nullptr;

    return ret;
}

void Arena::destroy() {
    clear();
    allocator.memory_release(&reservation);
    ZeroStruct(this);
}

ArenaMark Arena::mark() {
    return {
        .ptr = cur
    };
}

void Arena::restore(ArenaMark saved) {
    while (resources_stack && resources_stack->target >= saved.ptr) {
        resources_stack->drop(resources_stack->context, resources_stack->target);
        SllStackPop(resources_stack);
    }
    cur = saved.ptr;
    allocator.memory_commit_size(&reservation, cur - reservation.base);
}

void Arena::clear() {
    while (resources_stack) {
        resources_stack->drop(resources_stack->context, resources_stack->target);
        SllStackPop(resources_stack);
    }
    cur = reservation.base;
    allocator.memory_commit_size(&reservation, 0);
}

forall(T) T* Arena::alloc_one() {
    usize size = sizeof(T);

    cur      = (u8*)(((usize)cur + (alignof(T) - 1)) & ~(alignof(T) - 1));
    u8* ret  = cur;
    cur     += size;
    allocator.memory_commit_size(&reservation, cur - reservation.base);

    return (T*)memset(ret, 0, size);
}

void Arena::align() {
    constexpr usize MAX_ALIGN = 64;

    cur = (u8*)(((usize)cur + (MAX_ALIGN - 1)) & ~(MAX_ALIGN - 1));
}

forall(T) Slice<T> Arena::alloc_many(usize count) {
    if (count == 0) return Slice<T>{};

    usize size = sizeof(T) * count;

    cur      = (u8*)(((usize)cur + (alignof(T) - 1)) & ~(alignof(T) - 1));
    u8* ret  = cur;
    cur     += size;
    allocator.memory_commit_size(&reservation, cur - reservation.base);

    return Slice<T>{(T*)memset(ret, 0, size), count};
}

forall(T, U) T* Arena::alloc_resource(U* context, void (*drop)(U*, T*)) {
    T*                   result = alloc_one<T>();
    Arena::ResourceNode* node   = alloc_one<Arena::ResourceNode>();

    node->target  = result;
    node->drop    = drop;
    node->context = context;
    SllStackPush(resources_stack, node);
}

// -----------------------------------------------------------------------------

void arena_scratch_thread_local_create(MemoryAllocator allocator) {
    g_arena_scratch[0] = Arena::create(allocator);
    g_arena_scratch[1] = Arena::create(allocator);
}

void arena_scratch_thread_local_destroy() {
    g_arena_scratch[0].destroy();
    g_arena_scratch[1].destroy();
}

ScratchArena::ScratchArena() {
    arena = &g_arena_scratch[0];
    mark  = arena->mark();
}

ScratchArena::ScratchArena(Slice<Arena*> conflicts) {
    bool matched[2] = {};
    for (u32 i = 0; i < conflicts.count; ++i) {
        if (&g_arena_scratch[0] == conflicts.elems[i]) {
            matched[0] = true;
            if (matched[1]) goto err;
        } else if (&g_arena_scratch[1] == conflicts.elems[i]) {
            matched[1] = true;
            if (matched[0]) goto err;
        }
    }

    arena = &g_arena_scratch[matched[0] ? 1 : 0];
    mark  = arena->mark();

    return;
err:
    Panic("Both scratch arenas passed as conflicts to scratch_acquire");
}

ScratchArena::~ScratchArena() {
    arena->restore(mark);
}

// -----------------------------------------------------------------------------
}  // namespace