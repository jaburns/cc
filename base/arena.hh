#pragma once
#include "inc.hh"
namespace {

struct ArenaMark {
    u8* ptr;
};

class Arena {
    struct ResourceNode {
        ResourceNode* next;
        void*         target;
        void          (*drop)(void*);
    };

    MemoryReservation reservation     = {};
    ResourceNode*     resources_stack = {};
    u8*               cur             = {};

  public:
    MemoryAllocator allocator = {};

    static Arena create(MemoryAllocator allocator, usize block_size);
    void         destroy();

    ArenaMark mark();
    void      restore(ArenaMark saved);
    void      clear();

    forall(T) T* alloc_one();
    forall(T) Slice<T> alloc_many(usize count);
    forall(T) T* alloc_resource(void (*drop)(T*));
};

class ScratchArena : public NoCopy {
    ArenaMark mark = {};

  public:
    Arena* arena = {};

    ScratchArena();
    ScratchArena(Slice<Arena*> conflicts);
    ~ScratchArena();
};

void arena_scratch_thread_local_create(MemoryAllocator allocator, usize block_size);
void arena_scratch_thread_local_destroy();

}  // namespace