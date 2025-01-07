#pragma once

#ifndef DEBUG
#define DEBUG 0
#endif
#ifndef EDITOR
#define EDITOR 0
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

u64 operator""_kb(u64 n) { return n << 10; }
u64 operator""_mb(u64 n) { return n << 20; }
u64 operator""_gb(u64 n) { return n << 30; }

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

#define ZeroStruct(struct_ptr)            (decltype(struct_ptr))memset((struct_ptr), 0, sizeof(*(struct_ptr)))
#define CopyStruct(dest_ptr, struct_ptr)  (decltype(struct_ptr))memcpy((dest_ptr), (struct_ptr), sizeof(*(struct_ptr)))
#define ZeroArray(array, count)           memset((array), 0, (count) * sizeof((array)[0]))
#define CopyArray(dest_ptr, array, count) memcpy((dest_ptr), (array), (count) * sizeof((array)[0]))

#define X_forall_dispatch(_1, _2, _3, _4, name, ...) \
    name
#define X_forall_1(t1)             template <typename t1>
#define X_forall_2(t1, t2)         template <typename t1, typename t2>
#define X_forall_3(t1, t2, t3)     template <typename t1, typename t2, typename t3>
#define X_forall_4(t1, t2, t3, t4) template <typename t1, typename t2, typename t3, typename t4>
#define forall(...)                X_forall_dispatch(__VA_ARGS__, X_forall_4, X_forall_3, X_forall_2, X_forall_1)(__VA_ARGS__)

#if defined(__has_attribute)
#if __has_attribute(no_sanitize)
#define no_sanitize_overflow __attribute__((no_sanitize("unsigned-integer-overflow", "signed-integer-overflow")))
#else
#define no_sanitize_overflow
#endif
#else
#define no_sanitize_overflow
#endif

no_sanitize_overflow u32 wrapped_add(u32 a, u32 b) {
    return a + b;
}
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

forall(Fn) class X_Defer : Fn, NoCopy {
  public:
    X_Defer(Fn fn) : Fn(fn) {}
    ~X_Defer() { Fn::operator()(); }
};
#define x_defer_1(x, y, z) x##y##z
#define x_defer_0(line)    X_Defer x_defer_1(x__, line, __auto_drop) = [&](void) -> void
#define defer              x_defer_0(__LINE__)

forall(T) struct AtomicVal {
    T unsafe_inner;
    static_assert(std::atomic<T>::is_always_lock_free);
    std::atomic<T>& operator*() { return *(std::atomic<T>*)&this->unsafe_inner; }
    std::atomic<T>* ptr() { return (std::atomic<T>*)&this->unsafe_inner; }
};

forall(T) T max(T a, T b) { return a > b ? a : b; }
forall(T) T min(T a, T b) { return a < b ? a : b; }
forall(T) T clamp(T a, T min_, T max_) { return max(min_, min(max_, a)); }
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