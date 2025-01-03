#include "inc.hh"
namespace {

// -----------------------------------------------------------------------------
#define This Channel<T>

forall(T) This This::alloc(Arena* arena, usize capacity) {
    T*   buffers = arena->alloc_many<T>(2 * capacity).elems;
    This ret{};
    ret.capacity  = capacity;
    ret.buffer[0] = buffers;
    ret.buffer[1] = buffers + capacity;
    return ret;
}

forall(T) void This::push(T* item) {
    u32 cur_buf_reserve = std::atomic_fetch_add_explicit(cur_buffer_reserve.ptr(), 1, std::memory_order_relaxed);
    u32 buf             = cur_buf_reserve & 0x80000000 ? 1 : 0;
    u32 reserved_idx    = cur_buf_reserve & 0x7fffffff;

    if (reserved_idx >= capacity) {
        Panic("Channel is full");
    }

    T* target = &buffer[buf][reserved_idx];
    *target   = *item;

    std::atomic_fetch_add_explicit(count_commit[buf].ptr(), 1, std::memory_order_release);
}

#undef This
#define This ChannelIter<T>

forall(T) This This::start(Channel<T>* chan) {
    u32 prev_peek        = std::atomic_load_explicit(chan->cur_buffer_reserve.ptr(), std::memory_order_acquire);
    u32 new_value        = prev_peek & 0x80000000 ? 0 : 0x80000000;
    u32 prev_buf_reserve = std::atomic_exchange_explicit(chan->cur_buffer_reserve.ptr(), new_value, std::memory_order_acq_rel);

    u32 prev_buf          = prev_peek & 0x80000000 ? 1 : 0;
    u32 prev_reserved_idx = prev_buf_reserve & 0x7fffffff;

    for (;;) {
        u32 check = std::atomic_load_explicit(chan->count_commit[prev_buf].ptr(), std::memory_order_acquire);
        if (check == prev_reserved_idx) break;
    }

    *chan->count_commit[prev_buf] = 0;

    This it{};
    it.count  = prev_reserved_idx;
    it.idx    = -1;
    it.buffer = chan->buffer[prev_buf];

    it.next();

    return it;
}

forall(T) void This::next() {
    idx = wrapped_add(idx, 1);
    if (idx >= count) {
        done = true;
    }
}

#undef This
// -----------------------------------------------------------------------------
#if TEST

#define THREAD_COUNT 32
#define NUM_ITEMS    32768

global std::atomic_bool test_channel_start;
global std::atomic_bool test_channel_done;

void* test_channel_push_thread(void* arg) {
    while (!test_channel_start);

    Channel<u64>* chan = (Channel<u64>*)arg;
    for (u64 i = 1; i <= NUM_ITEMS; ++i) {
        chan->push(&i);
    }
    return nullptr;
}

void* test_channel_drain_thread(void* arg) {
    while (!test_channel_start);

    Channel<u64>* chan          = (Channel<u64>*)arg;
    u64           sum           = 0;
    bool          should_finish = false;

    for (;;) {
        for (auto it : *chan) {
            u64* item = (u64*)it.item;
            if (*item == 0) Panic("");
            sum += *item;
        }

        if (should_finish) break;
        if (test_channel_done) should_finish = true;
    }

    return (void*)(u64)sum;
}

void test_channel(void) {
    Arena arena = Arena::create(memory_get_global_allocator(), 0);

    for (i32 i = 0; i < 10; ++i) {
        test_channel_start = false;
        test_channel_done  = false;

        arena.clear();

        Channel<u64> chan = Channel<u64>::alloc(&arena, THREAD_COUNT * NUM_ITEMS);

        pthread_t threads[THREAD_COUNT];
        pthread_t drain_thread;

        pthread_create(&drain_thread, NULL, test_channel_drain_thread, (void*)&chan);
        for (u64 i = 0; i < THREAD_COUNT; ++i) {
            pthread_create(&threads[i], NULL, test_channel_push_thread, (void*)&chan);
        }

        test_channel_start = true;

        for (u64 i = 0; i < THREAD_COUNT; ++i) {
            pthread_join(threads[i], NULL);
        }

        test_channel_done = true;

        void* result;
        pthread_join(drain_thread, &result);
        u64 sum = (u64)result;

        u64 expected_sum = (u64)THREAD_COUNT * (NUM_ITEMS * (NUM_ITEMS + 1)) / 2;

        if (sum != expected_sum) {
            Panic("Expected %llu but got %llu\n", expected_sum, sum);
        }
    }

    arena.destroy();
}

#undef THREAD_COUNT
#undef NUM_ITEMS
#endif
// -----------------------------------------------------------------------------
}  // namespace