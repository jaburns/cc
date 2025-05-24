#include "inc.hh"
namespace a {
// -----------------------------------------------------------------------------
#define This Slice<T>

forall(T) T& This::operator[](usize index) {
    AssertM(index < count, "out of bounds access");
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

forall(T) usize This::size() {
    return sizeof(T) * count;
}

forall(T) void print_value(Arena* out, Slice<T>& slice) {
    print_value(out, slice.elems[0]);
    for (usize i = 1; i < slice.count; ++i) {
        print_value(out, ',');
        print_value(out, slice.elems[i]);
    }
}

#undef This
// -----------------------------------------------------------------------------
#define This Vec<T>

forall(T) This This::make(Arena* arena, usize capacity) {
    return {
        .elems = arena->push_many<T>(capacity).elems,
        .capacity = capacity,
    };
}

forall(T) This This::from_ptr(T* ptr, usize capacity) {
    return {
        .elems = ptr,
        .capacity = capacity,
    };
}

forall(T) T& This::operator[](usize index) {
    AssertM(index < count, "out of bounds access");
    return elems[index];
}

forall(T) Slice<T> This::slice() {
    return Slice<T>{elems, count};
}

forall(T) T* This::push() {
    AssertM(count < capacity, "attempted to push onto a full vec");
    return &elems[count++];
}

forall(T) T* This::pop() {
    AssertM(count > 0, "attempted to pop from an empty vec");
    return &elems[--count];
}

forall(T) void print_value(Arena* out, This& vec) {
    Slice<T> slice = vec.slice();
    print_value(out, slice);
}

#undef This
// -----------------------------------------------------------------------------
#define Template template <typename T, usize COUNT>
#define This Array<T, COUNT>

Template T& This::operator[](usize index) {
    AssertM(index < COUNT, "out of bounds access");
    return elems[index];
}

Template Slice<T> This::slice() {
    return Slice<T>{elems, COUNT};
}

Template void print_value(Arena* out, This& array) {
    Slice<T> slice = array.slice();
    print_value(out, slice);
}

#undef Template
#undef This
// -----------------------------------------------------------------------------
#define Template template <typename T, usize CAPACITY>
#define This InlineVec<T, CAPACITY>

Template T& This::operator[](usize index) {
    AssertM(index < count, "out of bounds access");
    return elems[index];
}

Template Slice<T> This::slice() {
    return Slice<T>{elems, count};
}

Template T* This::push() {
    AssertM(count < CAPACITY, "attempted to push onto a full InlineVec");
    return &elems[count++];
}

Template T* This::pop() {
    AssertM(count > 0, "attempted to pop from an empty InlineVec");
    return &elems[--count];
}

Template void print_value(Arena* out, This& vec) {
    Slice<T> slice = vec.slice();
    print_value(out, slice);
}

#undef Template
#undef This
// -----------------------------------------------------------------------------
#define This List<T>

forall(T) T* This::push(Arena* arena) {
    Elem* elem;
    if (free_list) {
        elem = free_list;
        free_list = elem->next;
        elem->next = nullptr;
    } else {
        elem = arena->push<Elem>();
    }

    if (!head) {
        head = elem;
        tail = elem;
    } else {
        elem->prev = tail;
        tail->next = elem;
        tail = elem;
    }

    count++;

    return &elem->item;
}

forall(T) void This::remove(T* elem) {
    static_assert(offsetof(Elem, item) == 0);
    Elem* e = (Elem*)elem;

    if (e->prev) {
        e->prev->next = e->next;
    } else {
        head = e->next;
    }

    if (e->next) {
        e->next->prev = e->prev;
    } else {
        tail = e->prev;
    }

    StructZero(e);
    e->next = free_list;
    free_list = e;

    count--;
}

forall(T) Slice<T> This::copy_into_array(Arena* arena) {
    Slice<T> ret = arena->push_many<T>(count);

    usize i = 0;
    foreach (it, iter()) {
        ret.elems[i++] = *it.item;
    }

    return ret;
}

forall(T) This::Iter This::Iter::make(This* target) {
    Iter ret = {};
    if (target->head) {
        ret.cur_elem = target->head;
        ret.next_elem = ret.cur_elem->next;
        ret.item = &ret.cur_elem->item;
    } else {
        ret.done = true;
    }
    return ret;
}

forall(T) void This::Iter::next() {
    cur_elem = next_elem;
    if (!cur_elem) {
        done = true;
        return;
    }
    next_elem = cur_elem ? cur_elem->next : nullptr;
    item = &cur_elem->item;
}

#undef This
// -----------------------------------------------------------------------------
}  // namespace a
