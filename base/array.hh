#pragma once
#include "inc.hh"
namespace {

class Arena;
forall(T) struct Slice;

// -----------------------------------------------------------------------------
#define Template template <typename T, usize SIZE>

Template struct Array {
    static constexpr usize count = SIZE;

    T elems[SIZE];

    T& operator[](usize index);

    Slice<T> slice();
};

#define Array(v0, ...) (Array<decltype(v0), sizeof((decltype(v0)[]){v0, __VA_ARGS__}) / sizeof(decltype(v0))>{{v0, __VA_ARGS__}})

#define RawArrayLen(arr) (sizeof(arr) / sizeof((arr)[0]))

#undef Template
// -----------------------------------------------------------------------------

forall(T) struct Slice {
    T*    elems;
    usize count;

    static Slice from_ptr(T* elems, usize count);

    forall(U) bool contains_all(Slice<U> search, bool (*compare)(T* a, U* b));

    T& operator[](usize index);
};

#define Slice(v0, ...) (Slice<decltype(v0)>::from_ptr((decltype(v0)[]){v0, __VA_ARGS__}, sizeof((decltype(v0)[]){v0, __VA_ARGS__}) / sizeof(decltype(v0))))

// TODO(jaburns) foreach iterator for array/slice/vec/inlinevec

// -----------------------------------------------------------------------------

forall(T) struct Vec {
    T*    elems;
    usize count;
    usize capacity;

    static Vec alloc(Arena* arena, usize capacity);
    static Vec from_ptr(T* ptr, usize capacity);

    T& operator[](usize index);

    Slice<T> slice();
    T*       push();
    T*       pop();
};

forall(T) void print_value(Vec<char>* out, Slice<T>& slice);
forall(T) void print_value(Vec<char>* out, Vec<T>& vec);

// -----------------------------------------------------------------------------
#define Template template <typename T, usize CAPACITY>

Template struct InlineVec {
    T     elems[CAPACITY];
    usize count;

    T& operator[](usize index);

    Slice<T> slice();
    T*       push();
    T*       pop();
};

Template void print_value(Vec<char>* out, InlineVec<T, CAPACITY>& vec);

#undef Template
// -----------------------------------------------------------------------------
}  // namespace