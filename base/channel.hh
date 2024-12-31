#pragma once
#include "inc.hh"
namespace {

// Multi-producer single-consumer channel for communicating between threads
//
// Anyone can push onto the channel from any thread at any time, but the same
// thread should always be responsible for draining it.

forall(T) class ChannelIter;

forall(T) class Channel {
    friend class ChannelIter<T>;

    usize          capacity           = {};
    AtomicVal<u32> count_commit[2]    = {};
    AtomicVal<u32> cur_buffer_reserve = {};
    T*             buffer[2]          = {};

  public:
    static Channel alloc(Arena* arena, usize capacity);
    void           push(T* item);

    ChannelIter<T> begin() { return ChannelIter<T>::start(this); }
    ChannelIter<T> end() { return ChannelIter<T>{}; }
};

forall(T) class ChannelIter {
  public:
    struct Entry {
        T* item;
    };

  private:
    u32   count  = {};
    u32   idx    = {};
    T*    buffer = {};
    Entry entry  = {};
    bool  done   = {};

  public:
    DefIteratorOperators(ChannelIter);
    static ChannelIter start(Channel<T>* chan);
    void               next();
};

}  // namespace