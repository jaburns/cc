#pragma once
#include "inc.hh"
namespace a {
// -----------------------------------------------------------------------------

// MurmurHash3_32
no_sanitize_overflow constexpr u32 hash32_bytes(const u8* data, usize len) {
    constexpr u32 seed = 0x87C263D1;

    int nblocks = len / 4;
    u32 h1 = seed;

    for (int i = 0; i < nblocks; i++) {
        u32 k1 =
            u32(data[i * 16 + 0]) |
            u32(data[i * 16 + 1]) << 8 |
            u32(data[i * 16 + 2]) << 16 |
            u32(data[i * 16 + 3]) << 24;

        k1 *= 0xCC9E2D51;
        k1 = (k1 << 15) | (k1 >> 17);
        k1 *= 0x1B873593;
        h1 ^= k1;
        h1 = (h1 << 13) | (h1 >> 19);
        h1 = h1 * 5 + 0xE6546B64;
    }

    const u8* tail = data + nblocks * 4;
    u32 k1 = 0;

    switch (len & 3) {
        case 3:
            k1 ^= tail[2] << 16;
        case 2:
            k1 ^= tail[1] << 8;
        case 1:
            k1 ^= tail[0];
            k1 *= 0xCC9E2D51;
            k1 = (k1 << 15) | (k1 >> 17);
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

template <usize N>
consteval u32 hash32(cchar (&str)[N]) {
    return hash32_bytes((u8*)str, N - 1);
}

// -----------------------------------------------------------------------------

no_sanitize_overflow constexpr u64 hash64_rotl64_(u64 x, int r) {
    return (x << r) | (x >> (64 - r));
}

no_sanitize_overflow constexpr u64 hash64_fmix64_(u64 k) {
    k ^= k >> 33;
    k *= 0xff51afd7ed558ccdULL;
    k ^= k >> 33;
    k *= 0xc4ceb9fe1a85ec53ULL;
    k ^= k >> 33;
    return k;
}

// MurmurHash3_x64_128
no_sanitize_overflow constexpr u64 hash64_bytes(const u8* data, usize len) {
    constexpr u64 c1 = 0x87c37b91114253d5ull;
    constexpr u64 c2 = 0x4cf5ad432745937full;
    constexpr u64 seed = 0x87C263D187C263D1ull;

    u64 h1 = seed;
    u64 h2 = seed;

    usize nblocks = len / 16;
    for (usize i = 0; i < nblocks; ++i) {
        u64 k1 =
            u64(data[i * 16 + 0]) |
            u64(data[i * 16 + 1]) << 8 |
            u64(data[i * 16 + 2]) << 16 |
            u64(data[i * 16 + 3]) << 24 |
            u64(data[i * 16 + 4]) << 32 |
            u64(data[i * 16 + 5]) << 40 |
            u64(data[i * 16 + 6]) << 48 |
            u64(data[i * 16 + 7]) << 56;
        u64 k2 =
            u64(data[i * 16 + 8]) |
            u64(data[i * 16 + 9]) << 8 |
            u64(data[i * 16 + 10]) << 16 |
            u64(data[i * 16 + 11]) << 24 |
            u64(data[i * 16 + 12]) << 32 |
            u64(data[i * 16 + 13]) << 40 |
            u64(data[i * 16 + 14]) << 48 |
            u64(data[i * 16 + 15]) << 56;

        k1 *= c1;
        k1 = hash64_rotl64_(k1, 31);
        k1 *= c2;
        h1 ^= k1;
        h1 = hash64_rotl64_(h1, 27);
        h1 += h2;
        h1 = h1 * 5 + 0x52dce729;

        k2 *= c2;
        k2 = hash64_rotl64_(k2, 33);
        k2 *= c1;
        h2 ^= k2;
        h2 = hash64_rotl64_(h2, 31);
        h2 += h1;
        h2 = h2 * 5 + 0x38495ab5;
    }

    const u8* tail = data + nblocks * 16;
    u64 k1 = 0, k2 = 0;
    switch (len & 15) {
        case 15:
            k2 ^= u64(tail[14]) << 48;
        case 14:
            k2 ^= u64(tail[13]) << 40;
        case 13:
            k2 ^= u64(tail[12]) << 32;
        case 12:
            k2 ^= u64(tail[11]) << 24;
        case 11:
            k2 ^= u64(tail[10]) << 16;
        case 10:
            k2 ^= u64(tail[9]) << 8;
        case 9:
            k2 ^= u64(tail[8]) << 0;
            k2 *= c2;
            k2 = hash64_rotl64_(k2, 33);
            k2 *= c1;
            h2 ^= k2;
        case 8:
            k1 ^= u64(tail[7]) << 56;
        case 7:
            k1 ^= u64(tail[6]) << 48;
        case 6:
            k1 ^= u64(tail[5]) << 40;
        case 5:
            k1 ^= u64(tail[4]) << 32;
        case 4:
            k1 ^= u64(tail[3]) << 24;
        case 3:
            k1 ^= u64(tail[2]) << 16;
        case 2:
            k1 ^= u64(tail[1]) << 8;
        case 1:
            k1 ^= u64(tail[0]) << 0;
            k1 *= c1;
            k1 = hash64_rotl64_(k1, 31);
            k1 *= c2;
            h1 ^= k1;
    }

    h1 ^= len;
    h2 ^= len;
    h1 += h2;
    // h2 += h1;
    h1 = hash64_fmix64_(h1);
    // h2 = hash64_fmix64_(h2);

    return h1;
}

template <usize N>
consteval u64 hash64(cchar (&str)[N]) {
    return hash64_bytes((u8*)str, N - 1);
}

u64 hash64_str(Str str) {
    return hash64_bytes((u8*)str.elems, str.count);
}

// -----------------------------------------------------------------------------
}  // namespace a
