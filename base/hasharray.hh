#pragma once
#include "inc.hh"
namespace {

forall(K, V) class HashArrayIter;

forall(K, V) class HashArray {
    static constexpr u32 LOAD_FACTOR_PERCENT = 70;

    friend class HashArrayIter<K, V>;

    u32* hashes     = {};
    K*   keys       = {};
    V*   values     = {};
    V*   value_stub = {};

  public:
    u32 capacity  = {};
    u32 max_elems = {};
    u32 count     = {};

    static HashArray alloc_with_cap(Arena* arena, u32 capacity);
    static HashArray alloc_with_elems(Arena* arena, u32 max_elems);

    V*   insert(K* key);
    V*   maybe_get(K* key);
    V*   get(K* key);
    V*   entry(K* key);
    bool remove(K* key);
    void clear();

    HashArrayIter<K, V> begin() { return HashArrayIter<K, V>::start(this); }
    HashArrayIter<K, V> end() { return HashArrayIter<K, V>{}; }

  private:
    static HashArray construct(Arena* arena, u32 capacity, u32 max_elems);

    u32 find_idx(K* key);
};

forall(K, V) class HashArrayIter {
  public:
    struct Entry {
        K* key;
        V* value;
    };

  private:
    u32              idx    = {};
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