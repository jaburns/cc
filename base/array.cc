#include "inc.hh"
namespace {
// --------------------------------------------------------------------------------------------------------------------
#define Template template <typename T, usize COUNT>
#define This     Array<T, COUNT>

Template T& This::operator[](usize index) {
    if (index >= COUNT) Panic("Out of bounds access");
    return elems[index];
}

Template Slice<T> This::slice() {
    return Slice<T>::from_ptr(elems, COUNT);
}

#undef Template
#undef This
// --------------------------------------------------------------------------------------------------------------------
#define This Slice<T>

forall(T) This This::from_ptr(T* elems, usize count) {
    Slice<T> ret = {};
    ret.elems    = elems;
    ret.count    = count;
    return ret;
}

forall(T) T& This::operator[](usize index) {
    if (index >= count) Panic("Out of bounds access");
    return elems[index];
}

forall(T) forall(U) bool This::contains_all(Slice<U> search, bool (*compare)(T* a, U* b)) {
    usize found = 0;
    for (usize i = 0; i < count; ++i) {
        for (usize j = 0; j < search.count; ++j) {
            if (compare(&elems[i], &search[j])) {
                if (++found == search.count) return true;
                break;
            }
        }
    }
    return false;
}

forall(T) void print_value(Vec<char>* out, Slice<T>& slice) {
    print_value(out, slice.elems[0]);
    for (usize i = 1; i < slice.count; ++i) {
        print_value(out, ",");
        print_value(out, slice.elems[i]);
    }
}

#undef This
// --------------------------------------------------------------------------------------------------------------------
#define This Vec<T>

forall(T) This This::alloc(Arena* arena, usize capacity) {
    This ret     = {};
    ret.elems    = arena->alloc_many<T>(capacity).elems;
    ret.capacity = capacity;
    return ret;
}

forall(T) This This::from_ptr(T* ptr, usize capacity) {
    This ret     = {};
    ret.elems    = ptr;
    ret.capacity = capacity;
    return ret;
}

forall(T) T& This::operator[](usize index) {
    if (index >= count) Panic("Out of bounds access");
    return elems[index];
}

forall(T) Slice<T> This::slice() {
    return Slice<T>::from_ptr(elems, count);
}

forall(T) T* This::push() {
    if (count == capacity) Panic("Attempted to push onto a full vec");
    return &elems[count++];
}

forall(T) T* This::pop() {
    if (count == 0) Panic("Attempted to pop from an empty vec");
    return &elems[--count];
}

forall(T) void print_value(Vec<char>* out, This& vec) {
    auto slice = vec.slice();
    print_value(out, slice);
}

#undef This
// --------------------------------------------------------------------------------------------------------------------
#define Template template <typename T, usize CAPACITY>
#define This     InlineVec<T, CAPACITY>

Template T& This::operator[](usize index) {
    if (index >= count) Panic("Out of bounds access");
    return elems[index];
}

Template Slice<T> This::slice() {
    return Slice<T>::from_ptr(elems, count);
}

Template T* This::push() {
    if (count == CAPACITY) Panic("Attempted to push onto a full InlineVec");
    return &elems[count++];
}

Template T* This::pop() {
    if (count == 0) Panic("Attempted to pop from an empty InlineVec");
    return &elems[--count];
}

Template void print_value(Vec<char>* out, This& vec) {
    auto slice = vec.slice();
    print_value(out, slice);
}

#undef Template
#undef This
// --------------------------------------------------------------------------------------------------------------------
}  // namespace