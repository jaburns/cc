#pragma once
#include "inc.hh"
namespace a {
// -----------------------------------------------------------------------------

konst char JSON_SERIALIZE_INDENTATION[256] = "                                                                                                                                                                                                                                                               ";

void json_chomp_whitespace(cchar* end, cchar** read);
bool json_expect_immediate(cchar* end, cchar** read, Str value);
bool json_expect(cchar* end, cchar** read, Str value);
bool json_skip_value(cchar* end, cchar** read);
bool json_skip_opened_string(cchar* end, cchar** read);

forall(T) bool json_from_file(Arena* base, Str path, T* obj);
forall(T) void json_to_file(Str path, T* obj);

#define DefJsonSerDe(ty)                               \
    void json_serialize(Arena* out, ty* val, u32 tab); \
    bool json_deserialize(Arena* arena, cchar* end, cchar** read, ty* val);

DefJsonSerDe(bool);
DefJsonSerDe(Str);
DefJsonSerDe(Str32);

DefJsonSerDe(u8);
DefJsonSerDe(u16);
DefJsonSerDe(u32);
DefJsonSerDe(u64);
DefJsonSerDe(i8);
DefJsonSerDe(i16);
DefJsonSerDe(i32);
DefJsonSerDe(i64);
DefJsonSerDe(usize);
DefJsonSerDe(isize);
DefJsonSerDe(float);
DefJsonSerDe(double);

DefJsonSerDe(vec2);
DefJsonSerDe(vec3);
DefJsonSerDe(vec4);
DefJsonSerDe(ivec2);
DefJsonSerDe(ivec3);
DefJsonSerDe(ivec4);
DefJsonSerDe(uvec2);
DefJsonSerDe(uvec3);
DefJsonSerDe(uvec4);

#undef DefJsonSerDe

forall(T) void json_serialize(Arena* out, Slice<T>* val, u32 tab);
forall(T) bool json_deserialize(Arena* arena, cchar* end, cchar** read, Slice<T>* val);

// -----------------------------------------------------------------------------
}  // namespace a
