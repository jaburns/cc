#pragma once
#include "inc.hh"
namespace a {
// -----------------------------------------------------------------------------

// Multi-producer single-consumer channel for communicating between threads
//
// Anyone can push onto the channel from any thread at any time, but the same
// thread should always be responsible for draining it.

forall(T) class Channel {
    class Iter {
        u32 count;
        u32 idx;
        T* buffer;

      public:
        bool done;
        T* item;
        func Iter make(Channel<T>* chan);
        void next();
    };

    usize capacity;
    AtomicVal<u32> count_commit[2];
    AtomicVal<u32> cur_buffer_reserve;
    T* buffer[2];

  public:
    func Channel make(Arena* arena, usize capacity);
    void push(T* item);
    Iter drain() { return Iter::make(this); }
};

// -----------------------------------------------------------------------------

#if TEST
void test_channel();
#endif

// -----------------------------------------------------------------------------
}  // namespace a