#include "inc.hh"
namespace {

#define ImplPrintValue(type, marker)                                                                 \
    void print_value(Vec<char>* out, type value) {                                                   \
        if (!out) {                                                                                  \
            printf(marker, value);                                                                   \
            return;                                                                                  \
        }                                                                                            \
        i32 written  = snprintf(&out->elems[out->count], out->capacity - out->count, marker, value); \
        out->count  += written;                                                                      \
        if (out->count > out->capacity) {                                                            \
            out->count = out->capacity;                                                              \
        }                                                                                            \
    }

ImplPrintValue(cchar*, "%s");
ImplPrintValue(char*, "%s");
ImplPrintValue(char, "%c");
ImplPrintValue(i8, "%i");
ImplPrintValue(u8, "%u");
ImplPrintValue(i16, "%i");
ImplPrintValue(u16, "%u");
ImplPrintValue(i32, "%i");
ImplPrintValue(u32, "%u");
ImplPrintValue(i64, "%lli");
ImplPrintValue(u64, "%llu");
ImplPrintValue(usize, "%lu");
ImplPrintValue(f32, "%f");
ImplPrintValue(f64, "%f");

#undef ImplPrintValue

void print_value(Vec<char>* out, bool value) {
    print_value(out, value ? "true" : "false");
}

void print() {}
forall(T, ... Args) void print(T value, Args... args) {
    print_value(nullptr, value);
    print(args...);
}
forall(... Args) void println(Args... args) {
    print(args...);
    print("\n");
}

void x_log_print() {}
forall(T, ... Args) void x_log_print(T value, Args... args) {
    print_value(nullptr, value);
    printf(" ");
    x_log_print(args...);
}

void str_print(Vec<char>* out) {}
forall(T, ... Args) void str_print(Vec<char>* out, T value, Args... args) {
    print_value(out, value);
    str_print(out, args...);
}
forall(... Args) void str_println(Vec<char>* out, Args... args) {
    str_print(out, args...);
    str_print(out, "\n");
}

void print_value(Vec<char>* out, Str value) {
    if (!out) {
        printf("%.*s", (i32)value.count, value.elems);
        return;
    }
    i32 written  = snprintf(&out->elems[out->count], out->capacity - out->count, "%.*s", (i32)value.count, value.elems);
    out->count  += written;
    if (out->count > out->capacity) {
        out->count = out->capacity;
    }
}

Str Str::from_cstr(cchar* cstr) {
    return Str{cstr, strlen(cstr)};
}

Str Str::from_nullable_cstr(cchar* nullable_cstr) {
    return nullable_cstr == nullptr ? Str{} : Str::from_cstr(nullable_cstr);
}

char* Str::to_cstr(Arena* arena) {
    char* ret = arena->alloc_many<char>(count + 1).elems;
    CopyArray(ret, elems, count);
    ret[count] = 0;
    return ret;
}

bool Str::eq_cstr(cchar* cstr) {
    i32 cmp = strncmp(elems, cstr, count);
    return cmp == 0 && cstr[count] == '\0';
}

bool Str::eq(Str other) {
    if (count != other.count) return false;
    return strncmp(elems, other.elems, count) == 0;
}

Str Str::clone(Arena* arena) {
    char* new_start = arena->alloc_many<char>(count).elems;
    CopyArray(new_start, elems, count);
    return Str{new_start, count};
}

Str Str::before_first_index(char split) {
    cchar* end = (cchar*)memchr(elems, split, count);
    if (end != nullptr) {
        return Str{elems, (usize)(end - elems)};
    }
    return *this;
}

Str Str::after_first_index(char split) {
    cchar* end = (cchar*)memchr(elems, split, count);
    if (end != nullptr) {
        usize fwd = end - elems + 1;
        return Str{elems + fwd, count - fwd};
    }
    return Str{};
}

Str Str::after_last_index(char split) {
    Str ret = {elems + count - 1, 1};
    while (ret.elems[0] != split) {
        ret.elems--;
        ret.count++;
        if (ret.elems == elems) {
            return *this;
        }
    }
    ret.elems++;
    if (ret.count > 0) ret.count--;
    return ret;
}

Str Str::trim() {
    Str str = *this;
    if (str.count == 0) return str;
    str.count--;
    while (str.count > 0 && isspace(*str.elems)) {
        str.elems++;
        str.count--;
    }
    cchar* str_end = str.elems + str.count;
    while (str.count > 0 && isspace(*str_end)) {
        str_end--;
        str.count--;
    }
    str.count++;
    return str;
}

bool Str::starts_with_cstr(cchar* cstr) {
    i32 i = 0;
    for (; i < count && cstr[i]; ++i) {
        if (elems[i] != cstr[i]) return false;
    }
    return !cstr[i];
}

Str Str::substr_to(usize idx) {
    Str str = *this;
    if (idx < str.count) str.count = idx;
    return str;
}

Str Str::substr_from(usize idx) {
    Str str = *this;
    if (idx >= str.count) return Str{};
    str.elems += idx;
    str.count -= idx;
    return str;
}

Str Str::substr(usize idx, usize len) {
    return substr_from(idx).substr_to(len);
}

u64 Str::parse_u64(i32 base) {
    char buffer[22];
    CopyArray(buffer, elems, min(21ul, count));
    buffer[count] = 0;
    return strtoull(buffer, nullptr, base);
}

i64 Str::parse_i64(i32 base) {
    char buffer[22];
    CopyArray(buffer, elems, min(21ul, count));
    buffer[count] = 0;
    return strtoll(buffer, nullptr, base);
}

u32 Str::parse_u32(i32 base) {
    char buffer[12];
    CopyArray(buffer, elems, min(11ul, count));
    buffer[count] = 0;
    return strtoul(buffer, nullptr, base);
}

i32 Str::parse_i32(i32 base) {
    char buffer[12];
    CopyArray(buffer, elems, min(11ul, count));
    buffer[count] = 0;
    return strtol(buffer, nullptr, base);
}

f32 Str::parse_f32() {
    char buffer[32];
    CopyArray(buffer, elems, min(31ul, count));
    buffer[count] = 0;
    return strtof(buffer, nullptr);
}

f64 Str::parse_f64() {
    char buffer[32];
    CopyArray(buffer, elems, min(31ul, count));
    buffer[count] = 0;
    return strtod(buffer, nullptr);
}

bool cstr_eq(cchar* a, cchar* b) {
    return strcmp(a, b) == 0;
}

void print_value(Vec<char>* out, U64PrintedWithCommas value) {
    char  buffer[32];
    char  out_buffer[32];
    i32   len    = snprintf(buffer, sizeof(buffer), "%llu", value.val);
    i32   commas = (len - 1) / 3;
    char* write  = out_buffer + len + commas;

    *write-- = '\0';

    for (i32 i = len - 1, c = 0; i >= 0; --i) {
        *write-- = buffer[i];
        if (++c == 3 && i != 0) {
            *write-- = ',';
            c        = 0;
        }
    }

    print_value(out, out_buffer);
}

}  // namespace