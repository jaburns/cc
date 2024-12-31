#pragma once
#include "inc.hh"
namespace {

#define ARENA_MIN_ALIGNMENT 16

struct ArenaMark {
    u8* ptr;
};

class Arena {
    struct ResourceNode {
        ResourceNode* next;
        void*         target;
        void          (*drop)(void*);
    };

    MemoryAllocator   allocator       = {};
    MemoryReservation reservation     = {};
    ResourceNode*     resources_stack = {};
    u8*               cur             = {};

  public:
    static Arena create(MemoryAllocator allocator, usize block_size);
    void         destroy();

    ArenaMark mark();
    void      restore(ArenaMark saved);
    void      clear();

    forall(T) T* alloc_one();
    forall(T) Slice<T> alloc_many(usize count);
    forall(T) T* alloc_resource(void (*drop)(T*));
};

class ScratchArenaHandle : public NoCopy {
  public:
    Arena*    arena = {};
    ArenaMark mark  = {};

    ScratchArenaHandle();
    ScratchArenaHandle(Slice<Arena*> conflicts);
    ~ScratchArenaHandle();
};

void arena_scratch_thread_local_create(MemoryAllocator allocator, usize block_size);

}  // namespace