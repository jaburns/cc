#pragma once
#include "inc.hh"
namespace a {
// -----------------------------------------------------------------------------
#if '1234' != 0x31323334
#error bindump module assumes little-endian platform
#endif
// -----------------------------------------------------------------------------

forall(T) void bin_to_file(Str path, T* obj);
forall(T) void bin_from_file(Arena* base, void* ctx, Str path, T* obj);

#define DefBinDumpSerDe(ty_)                  \
    void bin_serialize(Arena* out, ty_* val); \
    bool bin_deserialize(Arena* arena, void* ctx, u8* end, u8** read, ty_* val);

DefBinDumpSerDe(bool);
DefBinDumpSerDe(Str);

template <u8 CAPACITY>
void bin_serialize(Arena* out, InlineStr<CAPACITY>* val);
template <u8 CAPACITY>
bool bin_deserialize(Arena* arena, void* ctx, u8* end, u8** read, InlineStr<CAPACITY>* val);

DefBinDumpSerDe(u8);
DefBinDumpSerDe(u16);
DefBinDumpSerDe(u32);
DefBinDumpSerDe(u64);
DefBinDumpSerDe(i8);
DefBinDumpSerDe(i16);
DefBinDumpSerDe(i32);
DefBinDumpSerDe(i64);
DefBinDumpSerDe(usize);
DefBinDumpSerDe(isize);
DefBinDumpSerDe(float);
DefBinDumpSerDe(double);

DefBinDumpSerDe(vec2);
DefBinDumpSerDe(vec3);
DefBinDumpSerDe(vec4);
DefBinDumpSerDe(ivec2);
DefBinDumpSerDe(ivec3);
DefBinDumpSerDe(ivec4);
DefBinDumpSerDe(uvec2);
DefBinDumpSerDe(uvec3);
DefBinDumpSerDe(uvec4);

#undef DefBinDumpSerDe

forall(T) void bin_serialize(Arena* out, Slice<T>* val);
forall(T) bool bin_deserialize(Arena* arena, void* ctx, u8* end, u8** read, Slice<T>* val);

// -----------------------------------------------------------------------------
}  // namespace a
