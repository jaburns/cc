#include "inc.hh"
namespace a {
// -----------------------------------------------------------------------------
#define Template template <typename K, typename V, bool PREHASHED>
#define This HashArray_<K, V, PREHASHED>

Template This This::make(Arena* arena, u64 capacity, u64 max_elems) {
    u64* hashes = arena->push_many<u64>(capacity).elems;
    K* keys = arena->push_many<K>(capacity).elems;
    V* values = arena->push_many<V>(capacity).elems;
    V* value_stub = arena->push<V>();

    This map = {};

    map.capacity = capacity;
    map.max_elems = max_elems;

    map.count = 0;
    map.hashes = hashes;
    map.keys = keys;
    map.values = values;
    map.value_stub = value_stub;

    return map;
}

Template This This::make_with_cap(Arena* arena, u64 capacity) {
    capacity = next_power_of_2(capacity);
    u64 max_elems = capacity * LOAD_FACTOR_PERCENT / 100;
    return This::make(arena, capacity, max_elems);
}

Template This This::make_with_elems(Arena* arena, u64 max_elems) {
    u64 capacity = next_power_of_2(max_elems * 100 / LOAD_FACTOR_PERCENT);
    return This::make(arena, capacity, max_elems);
}

Template u64 This::find_idx(K* key) {
    u64 hash;
    if constexpr (PREHASHED) {
        hash = key->hash;
    } else {
        hash = hash64_bytes((u8*)key, sizeof(K));
    }
    if (hash < 2) hash += 2;
    u64 start_idx = hash & (capacity - 1);
    u64 i;

    for (i = start_idx; i < capacity; ++i) {
        if (hashes[i] == 0) return UINT64_MAX;
        if (hashes[i] == hash) return i;
    }
    for (i = 0; i < start_idx; ++i) {
        if (hashes[i] == 0) return UINT64_MAX;
        if (hashes[i] == hash) return i;
    }

    return UINT64_MAX;
}

Template V* This::insert(K* key) {
    AssertM(count < max_elems, "hasharray is full");

    u64 hash;
    if constexpr (PREHASHED) {
        hash = key->hash;
    } else {
        hash = hash64_bytes((u8*)key, sizeof(K));
    }
    if (hash < 2) hash += 2;

    u64 start_idx = hash & (capacity - 1);
    u64 i;

    for (i = start_idx; i < capacity; ++i) {
        if (hashes[i] < 2) goto found;
    }
    for (i = 0; i < start_idx; ++i) {
        if (hashes[i] < 2) goto found;
    }

    AssertUnreachable();

found:
    count++;
    hashes[i] = hash;
    keys[i] = *key;

    V* value = &values[i];
    return StructZero(value);
}

Template V* This::maybe_get(K* key) {
    u64 i = find_idx(key);
    return i == UINT64_MAX ? nullptr : &values[i];
}

Template V* This::get(K* key) {
    u64 i = find_idx(key);
    return i == UINT64_MAX ? value_stub : &values[i];
}

Template V* This::entry(K* key) {
    u64 hash;
    if constexpr (PREHASHED) {
        hash = key->hash;
    } else {
        hash = hash64_bytes((u8*)key, sizeof(K));
    }
    if (hash < 2) hash += 2;

    u64 start_idx = hash & (capacity - 1);
    u64 tombstone_idx = UINT64_MAX;
    u64 i;

#define X()                                                     \
    {                                                           \
        u64 stored_hash = hashes[i];                            \
        if (stored_hash > 1) {                                  \
            if (stored_hash == hash) return &values[i];         \
        } else if (stored_hash == 1) {                          \
            if (tombstone_idx == UINT64_MAX) tombstone_idx = i; \
        } else {                                                \
            goto not_found;                                     \
        }                                                       \
    }
    for (i = start_idx; i < capacity; ++i) X();
    for (i = 0; i < start_idx; ++i) X();
#undef X

    AssertUnreachable();

not_found:
    AssertM(count < max_elems, "hasharray is full");
    if (tombstone_idx < UINT64_MAX) i = tombstone_idx;

    count++;
    hashes[i] = hash;
    keys[i] = *key;

    V* value = &values[i];
    return StructZero(value);
}

Template bool This::remove(K* key) {
    u64 i = find_idx(key);
    if (i == UINT64_MAX) return false;

    count--;
    hashes[i] = 1;  // tombstone
    return true;
}

Template void This::clear() {
    count = 0;
    ArrayZero(hashes, capacity);
}

Template This::Iter This::Iter::make(This* map) {
    This::Iter ret = {};
    ret.idx = -1;
    ret.target = map;
    ret.next();
    return ret;
}

Template void This::Iter::next() {
    idx = wrapped_add(idx, 1ull);
    for (;;) {
        if (idx >= target->capacity) {
            done = true;
            return;
        }
        if (target->hashes[idx] > 1) {
            key = &target->keys[idx];
            item = &target->values[idx];
            return;
        }
        ++idx;
    }
}

#undef Template
#undef This
// -----------------------------------------------------------------------------
}  // namespace a
