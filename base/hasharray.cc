#include "inc.hh"
namespace {
// --------------------------------------------------------------------------------------------------------------------
#define This HashArray<K, V>

forall(K, V) This* This::construct(Arena* arena, usize capacity, usize max_elems) {
    u32*       hashes     = arena->alloc_many<u32>(capacity).elems;
    K*         keys       = arena->alloc_many<K>(capacity).elems;
    V*         values     = arena->alloc_many<V>(capacity).elems;
    V*         value_stub = arena->alloc_one<V>();
    HashArray* map        = arena->alloc_one<HashArray<K, V>>();

    map->capacity  = capacity;
    map->max_elems = max_elems;

    map->count      = 0;
    map->hashes     = hashes;
    map->keys       = keys;
    map->values     = values;
    map->value_stub = value_stub;

    return map;
}

forall(K, V) This* This::alloc_with_cap(Arena* arena, usize capacity) {
    capacity        = next_power_of_2(capacity);
    usize max_elems = capacity * HASHARRAY_LOAD_FACTOR_PERCENT / 100;
    return This::construct(arena, capacity, max_elems);
}

forall(K, V) This* This::alloc_with_elems(Arena* arena, usize max_elems) {
    usize capacity = next_power_of_2(max_elems * 100 / HASHARRAY_LOAD_FACTOR_PERCENT);
    return This::construct(arena, capacity, max_elems);
}

no_sanitize_overflow u32 hasharray_murmur3_32_hash(u8* bytes, usize len) {
    i32 nblocks = len / 4;
    u32 h1      = 0x87C263D1;  // seed

    for (i32 i = 0; i < nblocks; i++) {
        u32 k1;
        memcpy(&k1, bytes + i * 4, 4);
        k1 *= 0xCC9E2D51;
        k1  = (k1 << 15) | (k1 >> 17);
        k1 *= 0x1B873593;
        h1 ^= k1;
        h1  = (h1 << 13) | (h1 >> 19);
        h1  = h1 * 5 + 0xE6546B64;
    }

    u8* tail = bytes + nblocks * 4;
    u32 k1   = 0;

    switch (len & 3) {
        case 3:
            k1 ^= tail[2] << 16;
        case 2:
            k1 ^= tail[1] << 8;
        case 1:
            k1 ^= tail[0];
            k1 *= 0xCC9E2D51;
            k1  = (k1 << 15) | (k1 >> 17);
            k1 *= 0x1B873593;
            h1 ^= k1;
    };

    h1 ^= len;
    h1 ^= h1 >> 16;
    h1 *= 0x85EBCA6B;
    h1 ^= h1 >> 13;
    h1 *= 0xC2B2AE35;
    h1 ^= h1 >> 16;

    return h1;
}

forall(K, V) u32 This::find_idx(K* key) {
    u32 hash = hasharray_murmur3_32_hash((u8*)key, sizeof(K));
    if (hash < 2) hash += 2;
    u32 start_idx = hash & (capacity - 1);
    u32 i;

#define X()                                                                          \
    {                                                                                \
        u32 stored_hash = hashes[i];                                                 \
        if (stored_hash == 0) return UINT32_MAX;                                     \
        if (stored_hash > 1 && memcmp(key, (u8*)&keys[i], sizeof(K)) == 0) return i; \
    }

    for (i = start_idx; i < capacity; ++i) X();
    for (i = 0; i < start_idx; ++i) X();

#undef X

    return UINT32_MAX;
}

forall(K, V) V* This::insert(K* key) {
    if (count >= max_elems) {
        Panic("Fixed-size hasharray is full and cannot be resized");
    }

    u32 hash = hasharray_murmur3_32_hash((u8*)key, sizeof(K));
    if (hash < 2) hash += 2;

    u32 start_idx = hash & (capacity - 1);
    u32 i;

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
    keys[i]   = *key;

    V* value = &values[i];
    return ZeroStruct(value);
}

forall(K, V) V* This::maybe_get(K* key) {
    u32 i = find_idx(key);
    return i == UINT32_MAX ? nullptr : &values[i];
}

forall(K, V) V* This::get(K* key) {
    u32 i = find_idx(key);
    return i == UINT32_MAX ? value_stub : &values[i];
}

forall(K, V) V* This::entry(K* key) {
    u32 hash = hasharray_murmur3_32_hash(key, sizeof(K));
    if (hash < 2) hash += 2;

    u32 start_idx     = hash & (capacity - 1);
    u32 tombstone_idx = UINT32_MAX;
    u32 i;

#define X()                                                     \
    {                                                           \
        u32 stored_hash = hashes[i];                            \
        if (stored_hash > 1) {                                  \
            if (memcmp(key, &keys[i], sizeof(K)) == 0) {        \
                return &values[i];                              \
            }                                                   \
        } else if (stored_hash == 1) {                          \
            if (tombstone_idx == UINT32_MAX) tombstone_idx = i; \
        } else {                                                \
            goto not_found;                                     \
        }                                                       \
    }

    for (i = start_idx; i < capacity; ++i) X();
    for (i = 0; i < start_idx; ++i) X();

#undef X

    AssertUnreachable();

not_found:
    if (count >= max_elems) {
        Panic("Fixed-size hasharray is full and cannot be resized");
    }
    if (tombstone_idx < UINT32_MAX) i = tombstone_idx;

    count++;
    hashes[i] = hash;
    keys[i]   = *key;

    V* value = &values[i];
    return ZeroStruct(value);
}

forall(K, V) bool This::remove(K* key) {
    u32 i = find_idx(key);
    if (i == UINT32_MAX) return false;

    count--;
    hashes[i] = 1;  // tombstone
    return true;
}

forall(K, V) void This::clear() {
    count = 0;
    ZeroArray(hashes, capacity);
}

#undef This
// --------------------------------------------------------------------------------------------------------------------
#define This HashArrayIter<K, V>

forall(K, V) This This::start(HashArray<K, V>* map) {
    This ret   = {};
    ret.idx    = -1;
    ret.target = map;
    ret.next();
    return ret;
}

forall(K, V) void This::next() {
    idx = wrapped_add(idx, 1ull);
    for (;;) {
        if (idx >= target->capacity) {
            done = true;
            return;
        }
        if (target->hashes[idx] > 1) {
            entry.key   = &target->keys[idx];
            entry.value = &target->values[idx];
            return;
        }
        ++idx;
    }
}

#undef This
// --------------------------------------------------------------------------------------------------------------------
}  // namespace