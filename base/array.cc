#include "inc.hh"
namespace {
// -----------------------------------------------------------------------------
#define This Slice<T>

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

forall(T) void This::fill_copy(T* source) {
    for (usize i = 0; i < count; ++i) {
        elems[i] = *source;
    }
}

forall(T) void This::copy_into(void* mem) {
    memcpy(mem, elems, count * sizeof(T));
}

forall(T) forall(U) Slice<U> This::cast() {
    return Slice<U>{
        .elems = (U*)elems,
        .count = count * sizeof(T) / sizeof(U),
    };
}

forall(T) void print_value(Vec<char>* out, Slice<T>& slice) {
    print_value(out, '[');
    print_value(out, slice.elems[0]);
    for (usize i = 1; i < slice.count; ++i) {
        print_value(out, ',');
        print_value(out, slice.elems[i]);
    }
    print_value(out, ']');
}

#undef This
// -----------------------------------------------------------------------------
#define This Vec<T>

forall(T) This This::alloc(Arena* arena, usize capacity) {
    return {
        .elems    = arena->alloc_many<T>(capacity).elems,
        .capacity = capacity,
    };
}

forall(T) This This::from_ptr(T* ptr, usize capacity) {
    return {
        .elems    = ptr,
        .capacity = capacity,
    };
}

forall(T) T& This::operator[](usize index) {
    if (index >= count) Panic("Out of bounds access");
    return elems[index];
}

forall(T) Slice<T> This::slice() {
    return Slice<T>{elems, count};
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
    Slice<T> slice = vec.slice();
    print_value(out, slice);
}

#undef This
// -----------------------------------------------------------------------------
#define This GrowableVec<T>

forall(T) This::GrowableVec(MemoryAllocator allocator) {
    this->allocator = allocator;
    reservation     = allocator.memory_reserve();
    elems           = (T*)reservation.base;
    count           = 0;
}

forall(T) This::~GrowableVec() {
    allocator.memory_release(&reservation);
}

forall(T) T* This::push() {
    T* ret = &elems[count++];
    allocator.memory_commit_size(&reservation, sizeof(T) * count);
    return ret;
}

forall(T) Slice<T> This::copy_into_arena(Arena* arena) {
    Slice<T> ret = arena->alloc_many<T>(count);
    CopyArray(ret.elems, elems, count);
    return ret;
}

#undef This
// -----------------------------------------------------------------------------
#define Template template <typename T, usize COUNT>
#define This     Array<T, COUNT>

Template T& This::operator[](usize index) {
    if (index >= COUNT) Panic("Out of bounds access");
    return elems[index];
}

Template Slice<T> This::slice() {
    return Slice<T>{elems, COUNT};
}

Template void print_value(Vec<char>* out, This& array) {
    Slice<T> slice = array.slice();
    print_value(out, slice);
}

#undef Template
#undef This
// -----------------------------------------------------------------------------
#define Template template <typename T, usize CAPACITY>
#define This     InlineVec<T, CAPACITY>

Template T& This::operator[](usize index) {
    if (index >= count) Panic("Out of bounds access");
    return elems[index];
}

Template Slice<T> This::slice() {
    return Slice<T>{elems, count};
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
    Slice<T> slice = vec.slice();
    print_value(out, slice);
}

#undef Template
#undef This
// -----------------------------------------------------------------------------
}  // namespace