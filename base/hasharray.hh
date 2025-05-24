#pragma once
#include "inc.hh"
namespace a {
// -----------------------------------------------------------------------------
#define Template template <typename K, typename V, bool PREHASHED>

Template class HashArray_ {
    konst u32 LOAD_FACTOR_PERCENT = 70;

    u64* hashes;
    K* keys;
    V* values;
    V* value_stub;

  public:
    u64 capacity;
    u64 max_elems;
    u64 count;

    class Iter {
        u64 idx;
        HashArray_* target;

      public:
        K* key;
        V* item;
        bool done;

        func Iter make(HashArray_* map);
        void next();
    };

    func HashArray_ make_with_cap(Arena* arena, u64 capacity);
    func HashArray_ make_with_elems(Arena* arena, u64 max_elems);

    V* insert(K* key);
    V* maybe_get(K* key);
    V* get(K* key);
    V* entry(K* key);
    bool remove(K* key);
    void clear();

    Iter iter() { return Iter::make(this); }

  private:
    func HashArray_ make(Arena* arena, u64 capacity, u64 max_elems);

    u64 find_idx(K* key);
};

template <typename K, typename V>
using HashArray = HashArray_<K, V, false>;

template <typename K, typename V>
using PreHashArray = HashArray_<K, V, true>;

#undef Template
// -----------------------------------------------------------------------------
}  // namespace a