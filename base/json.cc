#include "inc.hh"
namespace a {
// -----------------------------------------------------------------------------

void json_chomp_whitespace(cchar* end, cchar** read) {
    while (*read < end && isspace(**read)) ++*read;
}

bool json_expect_immediate(cchar* end, cchar** read, Str value) {
    if (*read + value.count > end) return false;
    if (strncmp(*read, value.elems, value.count) == 0) {
        *read += value.count;
        return true;
    }
    return false;
}

bool json_expect(cchar* end, cchar** read, Str value) {
    json_chomp_whitespace(end, read);
    return json_expect_immediate(end, read, value);
}

bool json_skip_opened_object_or_array(cchar* end, cchar** read, cchar open, cchar close) {
    int depth = 1;
    bool in_string = false;
    while (depth > 0) {
        if (*read >= end) return false;
        char c = **read;
        if (in_string) {
            if (c == '\\') {
                ++*read;
            } else if (c == '"') {
                in_string = false;
            }
        } else {
            if (c == '"') {
                in_string = true;
            } else if (c == open) {
                ++depth;
            } else if (c == close) {
                --depth;
            }
        }
        ++*read;
    }
    return true;
}

bool json_skip_opened_string(cchar* end, cchar** read) {
    for (;;) {
        if (*read >= end) return false;
        char c = **read;
        ++*read;
        if (c == '\\') {
            ++*read;
        } else if (c == '"') {
            return true;
        }
    }
    return true;
}

bool json_skip_literal(cchar* end, cchar** read) {
    while (*read < end) {
        char c = **read;
        if (isspace(c) || c == ',' || c == '}' || c == ']')
            return true;
        ++*read;
    }
    return false;
}

bool json_skip_value(cchar* end, cchar** read) {
    json_chomp_whitespace(end, read);
    if (json_expect_immediate(end, read, "{")) {
        if (!json_skip_opened_object_or_array(end, read, '{', '}')) return false;
    } else if (json_expect_immediate(end, read, "[")) {
        if (!json_skip_opened_object_or_array(end, read, '[', ']')) return false;
    } else if (json_expect_immediate(end, read, "\"")) {
        if (!json_skip_opened_string(end, read)) return false;
    } else {
        if (!json_skip_literal(end, read)) return false;
        json_chomp_whitespace(end, read);
    }
    return true;
}

// -----------------------------------------------------------------------------

forall(T) void json_to_file(Str path, T* obj) {
    ScratchArena scratch{};
    auto sb = StrBuilder::make(scratch.arena);
    json_serialize(scratch.arena, obj, 0);
    fs_write_file_bytes(path, sb.to_str().to_slice().cast<u8>());
}

forall(T) void json_from_file(Arena* base, void* ctx, Str path, T* obj) {
    ScratchArena scratch(base);
    if (fs_file_exists(path)) {
        Str json = Str::from_slice_char(fs_read_file_bytes(scratch.arena, path).cast<char>());
        if (json_deserialize(base, ctx, json.elems + json.count, &json.elems, obj)) {
            return;
        }
        Panic("json_from_file: deserialize failed");
    }
    Panic("json_from_file: file does not exist");
}

// -----------------------------------------------------------------------------

void json_serialize(Arena* out, bool* val, u32 tab) {
    if (*val) {
        arena_print(out, "true");
    } else {
        arena_print(out, "false");
    }
}

bool json_deserialize(Arena* arena, void* ctx, cchar* end, cchar** read, bool* val) {
    json_chomp_whitespace(end, read);
    if (json_expect_immediate(end, read, "true")) {
        *val = true;
        return true;
    } else if (json_expect_immediate(end, read, "false")) {
        *val = false;
        return true;
    }
    log("json: expected bool value");
    return false;
}

void json_serialize(Arena* out, Str* val, u32 tab) {
    // TODO(jaburns) need to escape quotes and backslashes
    arena_print(out, '"', *val, '"');
}

bool json_deserialize(Arena* arena, void* ctx, cchar* end, cchar** read, Str* val) {
    bool escaping = false;
    u8* arena_start = arena->cur;

    if (!json_expect(end, read, "\"")) goto fail;

    for (;;) {
        if (!escaping) {
            if (**read == '\\') {
                escaping = true;
                ++*read;
                continue;
            } else if (**read == '"') {
                ++*read;
                break;
            }
        }
        escaping = false;
        *arena->push<char>() = **read;
        ++*read;
    }

    *val = Str{(char*)arena_start, (usize)(arena->cur - arena_start)};

    return true;
fail:
    log("json: failed to parse Str");
    return false;
}

template <u8 CAPACITY>
void json_serialize(Arena* out, InlineStr<CAPACITY>* val, u32 tab) {
    Str str = val->to_str();
    json_serialize(out, &str, tab);
}

template <u8 CAPACITY>
bool json_deserialize(Arena* arena, void* ctx, cchar* end, cchar** read, InlineStr<CAPACITY>* val) {
    ScratchArena scratch(arena);
    Str str = {};
    if (!json_deserialize(scratch.arena, ctx, end, read, &str)) goto fail;
    if (str.count > CAPACITY) goto fail;
    MemCopy(val->elems, str.elems, str.count);
    val->count = str.count;
    return true;
fail:
    log("json: failed to parse InlineStr");
    return false;
}

// -----------------------------------------------------------------------------

#define ImplJsonNumber(ty_, reader_)                                                     \
    void json_serialize(Arena* out, ty_* val, u32 tab) {                                 \
        arena_print(out, *val);                                                          \
    }                                                                                    \
    bool json_deserialize(Arena* arena, void* ctx, cchar* end, cchar** read, ty_* val) { \
        cchar* start = *read;                                                            \
        json_chomp_whitespace(end, read);                                                \
        ty_ parsed = (ty_)reader_(*read, (char**)read, 10);                              \
        if (*read == start) {                                                            \
            log("json: failed to parse number");                                         \
            return false;                                                                \
        }                                                                                \
        *val = parsed;                                                                   \
        return true;                                                                     \
    }

#define json_strtof(buf, read, base) strtof(buf, read)
#define json_strtod(buf, read, base) strtod(buf, read)

ImplJsonNumber(u8, strtoul);
ImplJsonNumber(u16, strtoul);
ImplJsonNumber(u32, strtoul);
ImplJsonNumber(u64, strtoull);
ImplJsonNumber(i8, strtol);
ImplJsonNumber(i16, strtol);
ImplJsonNumber(i32, strtol);
ImplJsonNumber(i64, strtoll);
ImplJsonNumber(usize, strtoull);
ImplJsonNumber(isize, strtoll);
ImplJsonNumber(float, json_strtof);
ImplJsonNumber(double, json_strtod);

#undef ImplJsonNumber

// -----------------------------------------------------------------------------

#define ImplJsonVec(ty_, underlying_, size_)                                             \
    void json_serialize(Arena* out, ty_* val, u32 tab) {                                 \
        Slice<underlying_> elems = Slice<underlying_>{(underlying_*)val, size_};         \
        json_serialize(out, &elems, tab);                                                \
    }                                                                                    \
    bool json_deserialize(Arena* arena, void* ctx, cchar* end, cchar** read, ty_* val) { \
        u8 buffer[2 * sizeof(ty_) * size_];                                              \
        Arena scratch = Arena::make_with_buffer(buffer, RawArrayLen(buffer));            \
        Slice<underlying_> slice;                                                        \
        json_deserialize(&scratch, ctx, end, read, &slice);                              \
        *val = *(ty_*)slice.elems;                                                       \
        return true;                                                                     \
    }

ImplJsonVec(vec2, float, 2);
ImplJsonVec(vec3, float, 3);
ImplJsonVec(vec4, float, 4);
ImplJsonVec(ivec2, i32, 2);
ImplJsonVec(ivec3, i32, 3);
ImplJsonVec(ivec4, i32, 4);
ImplJsonVec(uvec2, u32, 2);
ImplJsonVec(uvec3, u32, 3);
ImplJsonVec(uvec4, u32, 4);

#undef ImplJsonVec

// -----------------------------------------------------------------------------

forall(T) void json_serialize(Arena* out, Slice<T>* val, u32 tab) {
    arena_print(out, "[\n");

    tab++;
    Assert(2 * tab < sizeof(JSON_SERIALIZE_INDENTATION));
    Str tabstr = Str{JSON_SERIALIZE_INDENTATION, 2 * tab};

    int i = 0;
    foreach (it, val->iter()) {
        arena_print(out, tabstr);
        json_serialize(out, it.item, tab);
        if (++i < val->count) {
            arena_print(out, ",\n");
        } else {
            arena_print(out, "\n");
        }
    }

    tab--;
    tabstr = Str{JSON_SERIALIZE_INDENTATION, 2 * tab};
    arena_print(out, tabstr, ']');
}

forall(T) bool json_deserialize(Arena* arena, void* ctx, cchar* end, cchar** read, Slice<T>* val) {
    ScratchArena scratch(arena);
    scratch.arena->align<T>();
    T* first_elem = (T*)scratch.arena->cur;
    usize elems = 0;

    if (!json_expect(end, read, "[")) goto fail;

    for (;;) {
        if (json_expect(end, read, "]")) break;

        T* elem = scratch.arena->push<T>();
        if (!json_deserialize(arena, ctx, end, read, elem)) goto fail;
        elems++;

        json_chomp_whitespace(end, read);
        if (json_expect_immediate(end, read, "]")) break;
        if (!json_expect_immediate(end, read, ",")) goto fail;
    }

    *val = arena->push_many<T>(elems);
    ArrayCopy(val->elems, first_elem, elems);

    return true;
fail:
    log("json: failed to parse Slice<>");
    return false;
}

// -----------------------------------------------------------------------------
}  // namespace a
