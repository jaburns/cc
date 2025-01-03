#pragma once
#include "inc.hh"
namespace {

#define HASHARRAY_LOAD_FACTOR_PERCENT 70

forall(K, V) class HashArrayIter;

forall(K, V) class HashArray {
    friend class HashArrayIter<K, V>;

    u32* hashes     = {};
    K*   keys       = {};
    V*   values     = {};
    V*   value_stub = {};

  public:
    usize capacity  = {};
    usize max_elems = {};
    usize count     = {};

    static HashArray* alloc_with_cap(Arena* arena, usize capacity);
    static HashArray* alloc_with_elems(Arena* arena, usize max_elems);

    V*   insert(K* key);
    V*   maybe_get(K* key);
    V*   get(K* key);
    V*   entry(K* key);
    bool remove(K* key);
    void clear();

    HashArrayIter<K, V> begin() { return HashArrayIter<K, V>::start(this); }
    HashArrayIter<K, V> end() { return HashArrayIter<K, V>{}; }

  private:
    static HashArray* construct(Arena* arena, usize capacity, usize max_elems);

    u32 find_idx(K* key);
};

forall(K, V) class HashArrayIter {
  public:
    struct Entry {
        K* key;
        V* value;
    };

  private:
    usize            idx    = {};
    HashArray<K, V>* target = {};
    Entry            entry  = {};
    bool             done   = {};

  public:
    Entry          operator*() { return entry; }
    bool           operator!=(HashArrayIter& other) { return !done; }
    HashArrayIter& operator++() { return next(), *this; }

    static HashArrayIter start(HashArray<K, V>* map);
    void                 next();
};

}  // namespace