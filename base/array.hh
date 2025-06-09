#pragma once
#include "inc.hh"
namespace a {
// -----------------------------------------------------------------------------

class Arena;

// -----------------------------------------------------------------------------

forall(T) class ArrayIter {
    T* elems;
    usize count;
    usize idx;

  public:
    T* item;
    bool done;

    void next() {
        if (++idx < count) {
            item = &elems[idx];
        } else {
            done = true;
        }
    }

    func ArrayIter make(T* elems, usize count) {
        ArrayIter ret = {};
        ret.elems = elems;
        ret.count = count;
        ret.idx = 0;
        ret.item = elems;
        ret.done = count == 0;
        return ret;
    }
};

// -----------------------------------------------------------------------------

forall(T) struct Slice {
    T* elems;
    usize count;

    forall(U) bool contains_all(Slice<U> search, bool (*compare)(T* a, U* b));
    void fill_copy(T* source);
    void copy_into(void* mem);
    forall(U) Slice<U> cast();
    usize size();
    void clear_to_zero() { ZeroArray(elems, count); }

    T& operator[](usize index);

    ArrayIter<T> iter() { return ArrayIter<T>::make(elems, count); }
};

// -----------------------------------------------------------------------------

forall(T) struct Vec {
    T* elems;
    usize count;
    usize capacity;

    func Vec make(Arena* arena, usize capacity);
    func Vec from_ptr(T* ptr, usize capacity);

    T& operator[](usize index);

    ArrayIter<T> iter() { return ArrayIter<T>::make(elems, count); }

    Slice<T> slice();
    T* push();
    T* pop();
};

forall(T) void print_value(Arena* out, Slice<T>& slice);
forall(T) void print_value(Arena* out, Vec<T>& vec);

// -----------------------------------------------------------------------------
#define Template template <typename T, usize COUNT>

Template struct Array {
    konst usize count = COUNT;

    T elems[COUNT];

    T& operator[](usize index);

    ArrayIter<T> iter() { return ArrayIter<T>::make(elems, COUNT); }

    Slice<T> slice();
};

Template void print_value(Arena* out, Array<T, COUNT>& array);

#define RawArrayLen(array) (sizeof(array) / sizeof((array)[0]))
#define SliceFromRawArray(type, array) (Slice<type>{(array), RawArrayLen(array)})

#undef Template
// -----------------------------------------------------------------------------
#define Template template <typename T, usize CAPACITY>

Template struct InlineVec {
    konst usize capacity = CAPACITY;

    usize count;
    T elems[CAPACITY];

    T& operator[](usize index);

    ArrayIter<T> iter() { return ArrayIter<T>::make(elems, count); }

    Slice<T> slice();
    T* push();
    T* pop();
};

Template void print_value(Arena* out, InlineVec<T, CAPACITY>& vec);

#undef Template
// -----------------------------------------------------------------------------

forall(T) class List {
    struct Elem {
        T item;
        Elem* next;
        Elem* prev;
    };

    // it is safe to call List.remove(iter.item) while iterating
    class Iter {
        Elem* cur_elem;
        Elem* next_elem;

      public:
        bool done;
        T* item;

        func Iter make(List* target);
        void next();
    };

    Elem* head;
    Elem* tail;
    Elem* free_list;

  public:
    usize count;

    T* push(Arena* arena);
    void remove(T* elem);
    Slice<T> copy_into_array(Arena* arena);

    T* first() { return head ? &head->item : nullptr; }
    T* last() { return tail ? &tail->item : nullptr; }
    Iter iter() { return Iter::make(this); };
};

// -----------------------------------------------------------------------------
}  // namespace a
