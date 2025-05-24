#pragma once
// -----------------------------------------------------------------------------

#ifndef DEBUG
#define DEBUG 0
#endif
#ifndef EDITOR
#define EDITOR 0
#endif

#if defined(__APPLE__)
#define PLATFORM_APPLE 1
#if defined(TARGET_OS_MAC)
#define PLATFORM_MACOS 1
#endif
#endif

#if DEBUG
#ifdef _MSC_VER
#define Trap() __debugbreak()
#else
#include <signal.h>
#define Trap() raise(SIGTRAP)
#endif
#else
#define Trap()
#endif

#define global static
#define func static
#define konst static constexpr
#define local_persist static

#define foreach(iter, iterator) \
    for (auto iter = (iterator); !iter.done; iter.next())

#define X_forall_dispatch(_1, _2, _3, _4, name, ...) \
    name
#define X_forall_1(t1) template <typename t1>
#define X_forall_2(t1, t2) template <typename t1, typename t2>
#define X_forall_3(t1, t2, t3) template <typename t1, typename t2, typename t3>
#define X_forall_4(t1, t2, t3, t4) template <typename t1, typename t2, typename t3, typename t4>
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

static_assert(sizeof(char) == 1);
static_assert(sizeof(short) == 2);
static_assert(sizeof(int) == 4);
static_assert(sizeof(long long) == 8);

typedef unsigned char u8;
typedef signed char i8;
typedef unsigned short u16;
typedef signed short i16;
typedef unsigned int u32;
typedef signed int i32;
typedef unsigned long long u64;
typedef signed long long i64;

typedef ptrdiff_t isize;
typedef size_t usize;
typedef u32 bool32;
typedef u64 bool64;

#define cchar const char

#define USIZE_MAX SIZE_T_MAX

// -----------------------------------------------------------------------------
namespace a {
// -----------------------------------------------------------------------------

#define Panic(...)                                              \
    do {                                                        \
        fprintf(stderr, "PANIC @ %s:%i: ", __FILE__, __LINE__); \
        fprintf(stderr, __VA_ARGS__);                           \
        fprintf(stderr, "\n");                                  \
        Trap();                                                 \
        exit(EXIT_FAILURE);                                     \
    } while (0)

#define Assert(x) \
    if (!(x)) Panic("assert failed: " #x)
#define AssertM(x, ...) \
    if (!(x)) Panic(__VA_ARGS__)

#if DEBUG
#define DebugAssert(x) Assert(x)
#define DebugAssertM(...) AssertM(__VA_ARGS__)
#else
#define DebugAssert(x)
#define DebugAssertM(x)
#endif

consteval u64 operator""_kb(u64 n) { return n << 10; }
consteval u64 operator""_mb(u64 n) { return n << 20; }
consteval u64 operator""_gb(u64 n) { return n << 30; }

#if DEBUG
#define AssertUnreachable() Panic("unreachable!")
#else
#define AssertUnreachable() __builtin_unreachable()
#endif

#define Unimplemented() Panic("unimplemented!")

#define StructEq(a_ptr, b_ptr) (memcmp((a_ptr), (b_ptr), sizeof(*(a_ptr))) == 0)
#define StructZero(struct_ptr) ((decltype(struct_ptr))memset((struct_ptr), 0, sizeof(*(struct_ptr))))
#define ArrayZero(array, count) memset((array), 0, (count) * sizeof((array)[0]))
#define ArrayCopy(dest_ptr, array, count) memcpy((dest_ptr), (array), (count) * sizeof((array)[0]))
#define MemCopy memcpy

no_sanitize_overflow u32 wrapped_add(u32 a, u32 b) { return a + b; }
no_sanitize_overflow u32 wrapped_mul(u32 a, u32 b) { return a * b; }
no_sanitize_overflow u64 wrapped_add(u64 a, u64 b) { return a + b; }
no_sanitize_overflow u64 wrapped_mul(u64 a, u64 b) { return a * b; }

// any time we want to automatically run code at the end of a scope we should
// implement a struct that inherits from this one. conventionally, all structs
// and classes are trivially copyable, unless they inherit this one.
struct MagicScopeStruct {
    MagicScopeStruct() = default;
    MagicScopeStruct(const MagicScopeStruct&) = delete;
    MagicScopeStruct& operator=(const MagicScopeStruct&) = delete;
    MagicScopeStruct(MagicScopeStruct&&) = delete;
    MagicScopeStruct& operator=(MagicScopeStruct&&) = delete;
    void* operator new(usize) = delete;
    void* operator new[](usize) = delete;
};

forall(Fn) class X_Defer : Fn, MagicScopeStruct {
  public:
    X_Defer(Fn fn) : Fn(fn) {}
    ~X_Defer() { Fn::operator()(); }
};
#define x_defer_1(x, y, z) x##y##z
#define x_defer_0(line) X_Defer x_defer_1(x__, line, __auto_drop) = [&](void) -> void
#define defer x_defer_0(__LINE__)

forall(T) struct AtomicVal {
    T unsafe_inner;
    static_assert(::std::atomic<T>::is_always_lock_free);
    ::std::atomic<T>& operator*() { return *(::std::atomic<T>*)&this->unsafe_inner; }
    ::std::atomic<T>* ptr() { return (::std::atomic<T>*)&this->unsafe_inner; }
};

#define Swap(a, b)     \
    do {               \
        auto temp = b; \
        b = a;         \
        a = temp;      \
    } while (0)

forall(T) T max(T a, T b) { return a > b ? a : b; }
forall(T) T min(T a, T b) { return a < b ? a : b; }
forall(T) T clamp(T a, T min_, T max_) { return max(min_, min(max_, a)); }
forall(T) T clamp01(T a) { return max((T)0, min((T)1, a)); }
forall(T) int sign(T a) { return a >= 0 ? 1 : -1; }
int next_power_of_2(int a) { return a <= 1 ? 1 : 1u << (32 - __builtin_clz(a - 1)); }
int count_leading_zeroes(u64 a) { return __builtin_clzll(a); }
int count_leading_zeroes(u32 a) { return __builtin_clz(a); }

forall(T, U) U bit_cast(T a) {
    union {
        T in;
        U out;
    } bits = {};
    bits.in = a;
    return bits.out;
}
#define HackCast(ty, obj) (bit_cast<decltype(obj), ty>(obj))

// -----------------------------------------------------------------------------
}  // namespace a
