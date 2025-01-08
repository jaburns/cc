#pragma once
#include "inc.hh"
namespace {
// -----------------------------------------------------------------------------

class Arena;

// -----------------------------------------------------------------------------

forall(T) class ArrayIter {
  private:
    T*    elems = {};
    usize count = {};
    usize idx   = {};

  public:
    T&         operator*() { return elems[idx]; }
    bool       operator!=(ArrayIter& other) { return idx < count; }
    ArrayIter& operator++() { return idx++, *this; }

    static ArrayIter start(T* elems, usize count) {
        ArrayIter ret = {};
        ret.elems     = elems;
        ret.count     = count;
        return ret;
    }
};

// -----------------------------------------------------------------------------

forall(T) struct Slice {
    T*    elems;
    usize count;

    forall(U) bool contains_all(Slice<U> search, bool (*compare)(T* a, U* b));
    void fill_copy(T* source);
    void copy_into(void* mem);
    forall(U) Slice<U> cast();

    T& operator[](usize index);

    ArrayIter<T> begin() { return ArrayIter<T>::start(elems, count); }
    ArrayIter<T> end() { return ArrayIter<T>{}; }
};

// -----------------------------------------------------------------------------

forall(T) struct Vec {
    T*    elems;
    usize count;
    usize capacity;

    static Vec alloc(Arena* arena, usize capacity);
    static Vec from_ptr(T* ptr, usize capacity);

    T& operator[](usize index);

    ArrayIter<T> begin() { return ArrayIter<T>::start(elems, count); }
    ArrayIter<T> end() { return ArrayIter<T>{}; }

    Slice<T> slice();
    T*       push();
    T*       pop();
};

forall(T) void print_value(Vec<char>* out, Slice<T>& slice);
forall(T) void print_value(Vec<char>* out, Vec<T>& vec);

// -----------------------------------------------------------------------------
#define Template template <typename T, usize COUNT>

Template struct Array {
    static constexpr usize count = COUNT;

    T elems[COUNT];

    T& operator[](usize index);

    ArrayIter<T> begin() { return ArrayIter<T>::start(elems, COUNT); }
    ArrayIter<T> end() { return ArrayIter<T>{}; }

    Slice<T> slice();
};

Template void print_value(Vec<char>* out, Array<T, COUNT>& array);

#define RawArrayLen(array)             (sizeof(array) / sizeof((array)[0]))
#define SliceFromRawArray(type, array) (Slice<type>{(array), RawArrayLen(array)})

#undef Template
// -----------------------------------------------------------------------------
#define Template template <typename T, usize CAPACITY>

Template struct InlineVec {
    static constexpr usize capacity = CAPACITY;

    T     elems[CAPACITY];
    usize count;

    T& operator[](usize index);

    ArrayIter<T> begin() { return ArrayIter<T>::start(elems, count); }
    ArrayIter<T> end() { return ArrayIter<T>{}; }

    Slice<T> slice();
    T*       push();
    T*       pop();
};

Template void print_value(Vec<char>* out, InlineVec<T, CAPACITY>& vec);

#undef Template
// -----------------------------------------------------------------------------
}  // namespace