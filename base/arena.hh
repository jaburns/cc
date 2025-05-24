#pragma once
#include "inc.hh"
namespace a {

struct Arena {
    MemoryReservation reservation;
    u8* cur;
    bool using_reservation;
    // ---

    void create(usize reserve_size = 0);
    void destroy();

    func Arena make_with_buffer(u8* bytes, usize count);

    void clear() { cur = reservation.base; }
    void release_unused_pages();

    void align(usize alignment);
    forall(T) void align();
    forall(T) T* push();
    forall(T) Slice<T> push_many(usize count);
    void* push_mem(void* start, usize size);
};

struct ScratchArena : MagicScopeStruct {
    u8* arena_mark;
    Arena* arena;

    ScratchArena();
    ScratchArena(Arena* conflict);
    ScratchArena(Slice<Arena*> conflicts);
    ~ScratchArena();

  private:
    void init(Slice<Arena*> conflicts);
};

void arena_bind_global_scratch(Arena* scratch0, Arena* scratch1);

}  // namespace a
