#pragma once
#include "inc.hh"
namespace a {
// -----------------------------------------------------------------------------

class Arena {
    u8* base;
    usize pages_committed;
    usize reserved_size;

  public:
    u8* cur;
    // ---

    func void global_init();
    func void thread_init(Arena* scratch0, Arena* scratch1);

    func Arena make_with_buffer(u8* bytes, usize count);
    void create(usize reserve_size = 0);
    void destroy();

    void clear() { cur = base; }

    void max_align();
    forall(T) void align();
    forall(T) T* push();
    forall(T) T* push_unaligned();
    forall(T) Slice<T> push_many(usize count);
    forall(T) Slice<T> push_many_unaligned(usize count);
    void push_bytes(void* start, usize size);

  private:
    global u32 log_reserve_page_size;
    global u32 log_commit_page_size;

    void bump(usize size);

    u8* mem_reserve(usize size);
    void mem_commit(u8* ptr, usize size);
    void mem_release(u8* ptr, usize size);
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

// -----------------------------------------------------------------------------
}  // namespace a
