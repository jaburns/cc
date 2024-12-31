#pragma once

#ifndef DEBUG
#define DEBUG 0
#endif

#if DEBUG
#ifdef _MSC_VER
#define Trap() __debugbreak()
#else
#include <csignal>
#define Trap() raise(SIGTRAP)
#endif
#else
#define Trap()
#endif

#define global
#define readonly_global
#define local_persist static
#define no_return     [[noreturn]]
#define no_inline     __attribute__((noinline))

typedef uint8_t   u8;
typedef int8_t    i8;
typedef uint16_t  u16;
typedef int16_t   i16;
typedef uint32_t  u32;
typedef int32_t   i32;
typedef uint64_t  u64;
typedef int64_t   i64;
typedef float     f32;
typedef double    f64;
typedef ptrdiff_t isize;
typedef size_t    usize;
#define cchar const char

#define Assert(x)                                                                  \
    do {                                                                           \
        if (!(x)) {                                                                \
            fprintf(stderr, "ASSERTION FAILED @ %s:%i : " #x, __FILE__, __LINE__); \
            Trap();                                                                \
            exit(EXIT_FAILURE);                                                    \
        }                                                                          \
    } while (0)

#if DEBUG
#define DebugAssert(x) Assert(x)
#else
#define DebugAssert(x)
#endif

#define Kb(n) (((u64)(n)) << 10)
#define Mb(n) (((u64)(n)) << 20)
#define Gb(n) (((u64)(n)) << 30)

#define Stringify(x)      #x
#define Concatenate(x, y) x##y

#define Panic(...)                                              \
    do {                                                        \
        fprintf(stderr, "PANIC @ %s:%i: ", __FILE__, __LINE__); \
        fprintf(stderr, __VA_ARGS__);                           \
        fprintf(stderr, "\n");                                  \
        Trap();                                                 \
        exit(EXIT_FAILURE);                                     \
    } while (0)

#if DEBUG
#define AssertUnreachable() Panic("Unreachable!")
#else
#define AssertUnreachable() __builtin_unreachable()
#endif

#define Unimplemented() Panic("Unimplemented!")

#define ZeroStruct(struct_ptr)      (decltype(struct_ptr))memset((struct_ptr), 0, sizeof(*(struct_ptr)))
#define ZeroArray(array_ptr, count) memset((array_ptr), 0, (count) * sizeof((array_ptr)[0]))

#define X_forall_1(t1)             template <typename t1>
#define X_forall_2(t1, t2)         template <typename t1, typename t2>
#define X_forall_3(t1, t2, t3)     template <typename t1, typename t2, typename t3>
#define X_forall_4(t1, t2, t3, t4) template <typename t1, typename t2, typename t3, typename t4>

#define X_forall_dispatch(_1, _2, _3, _4, name, ...) name

#define forall(...) X_forall_dispatch(__VA_ARGS__, X_forall_4, X_forall_3, X_forall_2, X_forall_1)(__VA_ARGS__)

#if defined(__has_attribute)
#if __has_attribute(no_sanitize)
#define no_sanitize_overflow __attribute__((no_sanitize("unsigned-integer-overflow", "signed-integer-overflow")))
#else
#define no_sanitize_overflow
#endif
#else
#define no_sanitize_overflow
#endif

no_sanitize_overflow u32 wrapped_add(u32 a, u32 b) { return a + b; }
no_sanitize_overflow u32 wrapped_mul(u32 a, u32 b) { return a * b; }
no_sanitize_overflow u64 wrapped_add(u64 a, u64 b) { return a + b; }
no_sanitize_overflow u64 wrapped_mul(u64 a, u64 b) { return a * b; }

struct NoCopy {
    NoCopy()                         = default;
    NoCopy(const NoCopy&)            = delete;
    NoCopy& operator=(const NoCopy&) = delete;
    NoCopy(NoCopy&&)                 = delete;
    NoCopy& operator=(NoCopy&&)      = delete;
};

#define DefIteratorOperators(T)                        \
    Entry operator*() { return this->entry; }          \
    bool  operator!=(T& other) { return !this->done; } \
    T&    operator++() {                               \
        this->next();                               \
        return *this;                               \
    }

forall(T) struct AtomicVal {
    T val;
    static_assert(std::atomic<T>::is_always_lock_free);
    std::atomic<T>* as_atomic() { return (std::atomic<T>*)&this->val; }
};

forall(T) T max(T a, T b) { return a > b ? a : b; }
forall(T) T min(T a, T b) { return a < b ? a : b; }
forall(T) T clamp(T a, T min, T max) { return max(min, min(max, a)); }
forall(T) T clamp01(T a) { return max((T)0, min((T)1, a)); }
forall(T) i32 sign(T a) { return a >= 0 ? 1 : -1; }
i32 next_power_of_2(i32 a) { return a <= 1 ? 1 : 1u << (32 - __builtin_clz(a - 1)); }
i32 count_leading_zeroes(u64 a) { return __builtin_clzll(a); }
i32 count_leading_zeroes(u32 a) { return __builtin_clz(a); }

#define SllStackPush(stack_head, node) \
    do {                               \
        (node)->next = (stack_head);   \
        (stack_head) = (node);         \
    } while (0)

#define SllStackPop(stack_head)            \
    do {                                   \
        (stack_head) = (stack_head)->next; \
    } while (0)

#define SllStackPopInto(stack_head, node)  \
    do {                                   \
        (node)       = (stack_head);       \
        (stack_head) = (stack_head)->next; \
        (node)->next = NULL;               \
    } while (0)