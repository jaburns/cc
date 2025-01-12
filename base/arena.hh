#pragma once
#include "inc.hh"
namespace {

struct ArenaMark {
    u8* ptr;
};

class Arena {
    struct ResourceNode {
        ResourceNode* next;
        void*         context;
        void*         target;
        void          (*drop)(void* context, void* target);
    };

    MemoryReservation reservation     = {};
    ResourceNode*     resources_stack = {};
    u8*               cur             = {};

  public:
    MemoryAllocator allocator = {};

    static Arena create(MemoryAllocator allocator);
    void         destroy();

    ArenaMark mark();
    void      restore(ArenaMark saved);
    void      clear();
    void      align();

    forall(T) T* alloc_one();
    forall(T) Slice<T> alloc_many(usize count);
    forall(T, U) T* alloc_resource(U* context, void (*drop)(U*, T*));
};

class ScratchArena : NoCopy {
    ArenaMark mark = {};

  public:
    Arena* arena = {};

    ScratchArena();
    ScratchArena(Slice<Arena*> conflicts);
    ~ScratchArena();
};

void arena_scratch_thread_local_create(MemoryAllocator allocator);
void arena_scratch_thread_local_destroy();

}  // namespace