#pragma once
#include "inc.hh"
namespace a {
// -----------------------------------------------------------------------------

forall(T) bool bin_from_file(Arena* base, Str path, T* obj) {
    ScratchArena scratch(base);
    if (fs_file_exists(path)) {
        Slice<u8> bin = fs_read_file_bytes(scratch.arena, path);
        if (bin_deserialize(base, bin.elems + bin.count, &bin.elems, obj)) {
            return true;
        }
    }
    StructZero(obj);
    return false;
}

forall(T) void bin_to_file(Str path, T* obj) {
    ScratchArena scratch{};
    u8* start = scratch.arena->cur;
    bin_serialize(scratch.arena, obj);
    fs_write_file_bytes(path, Slice<u8>{start, (usize)(scratch.arena->cur - start)});
}

// -----------------------------------------------------------------------------

void bin_serialize(Arena* out, bool* val) {
    *out->push<u8>() = *val ? 1 : 0;
}

bool bin_deserialize(Arena* arena, u8* end, u8** read, bool* val) {
    bool ok = *read < end;
    *val = *(read++) != 0;
    return ok;
}

void bin_serialize(Arena* out, Str* val) {
    u32 count = val->count;
    MemCopy(out->push_unaligned<u32>(), &count, sizeof(u32));
    char* buffer = out->push_many<char>(val->count).elems;
    MemCopy(buffer, val->elems, val->count);
}

bool bin_deserialize(Arena* arena, u8* end, u8** read, Str* val) {
    u32 count = 0;
    if (!bin_deserialize(arena, end, read, &count)) return false;
    if (*read + count > end) return false;
    char* buffer = arena->push_many<char>(count).elems;
    MemCopy(buffer, *read, count);
    *read += count;
    *val = Str{buffer, count};
    return true;
}

void bin_serialize(Arena* out, Str32* val) {
    u8 count = strnlen(val->elems, 32);
    *out->push<u8>() = count;
    char* buffer = out->push_many<char>(count).elems;
    MemCopy(buffer, val->elems, count);
}

bool bin_deserialize(Arena* arena, u8* end, u8** read, Str32* val) {
    u8 count = 0;
    if (!bin_deserialize(arena, end, read, &count)) return false;
    if (count >= 32 || *read + count > end) return false;
    *val = Str32{};
    MemCopy(val->elems, *read, count);
    *read += count;
    return true;
}

// -----------------------------------------------------------------------------

#define ImplBinCopy(ty_)                                               \
    void bin_serialize(Arena* out, ty_* val) {                         \
        MemCopy(out->push_unaligned<ty_>(), val, sizeof(ty_));         \
    }                                                                  \
    bool bin_deserialize(Arena* arena, u8* end, u8** read, ty_* val) { \
        if (*read + sizeof(ty_) > end) return false;                   \
        MemCopy(val, *read, sizeof(ty_));                              \
        *read += sizeof(ty_);                                          \
        return true;                                                   \
    }

ImplBinCopy(u8);
ImplBinCopy(u16);
ImplBinCopy(u32);
ImplBinCopy(u64);
ImplBinCopy(i8);
ImplBinCopy(i16);
ImplBinCopy(i32);
ImplBinCopy(i64);
ImplBinCopy(usize);
ImplBinCopy(isize);
ImplBinCopy(float);
ImplBinCopy(double);

ImplBinCopy(vec2);
ImplBinCopy(vec3);
ImplBinCopy(vec4);
ImplBinCopy(ivec2);
ImplBinCopy(ivec3);
ImplBinCopy(ivec4);
ImplBinCopy(uvec2);
ImplBinCopy(uvec3);
ImplBinCopy(uvec4);

#undef ImplBinCopy

// -----------------------------------------------------------------------------

forall(T) void bin_serialize(Arena* out, Slice<T>* val) {
    u32 count = val->count;
    MemCopy(out->push_unaligned<u32>(), &count, sizeof(u32));
    for (u32 i = 0; i < val->count; ++i) {
        bin_serialize(out, &val->elems[i]);
    }
}

forall(T) bool bin_deserialize(Arena* arena, u8* end, u8** read, Slice<T>* val) {
    u32 count = 0;
    if (!bin_deserialize(arena, end, read, &count)) return false;
    *val = arena->push_many<T>(count);
    for (u32 i = 0; i < val->count; ++i) {
        if (!bin_deserialize(arena, end, read, &val->elems[i])) return false;
    }
    return true;
}

// -----------------------------------------------------------------------------
}  // namespace a
