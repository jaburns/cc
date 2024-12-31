#pragma once
#include "inc.hh"
namespace {

class Arena;

// -----------------------------------------------------------------------------
#define Template template <typename T, usize SIZE>

Template class Array {
  public:
    T elems[SIZE];

    T& operator[](usize index);
};

#undef Template
// -----------------------------------------------------------------------------

forall(T) class Slice {
  public:
    T*    elems = {};
    usize count = {};

    static Slice from_ptr(T* elems, usize count);

    T& operator[](usize index);
};

#define SliceLit(T, ...) (Slice<T>::from_ptr((T[]){__VA_ARGS__}, sizeof((T[]){__VA_ARGS__}) / sizeof(T)))

// -----------------------------------------------------------------------------

forall(T) class Vec {
  public:
    T*    elems    = {};
    usize count    = {};
    usize capacity = {};

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

Template class InlineVec {
  public:
    T     elems[CAPACITY] = {};
    usize count           = {};

    T& operator[](usize index);

    Slice<T> slice();
    T*       push();
    T*       pop();
};

Template void print_value(Vec<char>* out, InlineVec<T, CAPACITY>& vec);

#undef Template
// -----------------------------------------------------------------------------
}  // namespace