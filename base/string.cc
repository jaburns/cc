#include "inc.hh"
namespace a {
// -----------------------------------------------------------------------------

#define ImplPrintValue(type, marker)                                                           \
    void print_value(Arena* out, type value) {                                                 \
        if (!out) {                                                                            \
            printf(marker, value);                                                             \
            return;                                                                            \
        }                                                                                      \
        u8* arena_start = out->cur;                                                            \
        /* re 256: doesn't need to be very big since this function handles printing individual \
           literal values and strings are handled separately. */                               \
        Slice<char> out_buffer = out->push_many<char>(256);                                    \
        int written = snprintf(out_buffer.elems, out_buffer.count, marker, value);             \
        out->cur = arena_start + written;                                                      \
    }
ImplPrintValue(i8, "%i");
ImplPrintValue(u8, "%u");
ImplPrintValue(i16, "%i");
ImplPrintValue(u16, "%u");
ImplPrintValue(i32, "%i");
ImplPrintValue(u32, "%u");
ImplPrintValue(i64, "%lli");
ImplPrintValue(u64, "%llu");
ImplPrintValue(usize, "%lu");
ImplPrintValue(isize, "%li");
ImplPrintValue(float, "%f");
ImplPrintValue(double, "%f");
#undef ImplPrintValue

#define ImplPrintValueStr(type)                                    \
    void print_value(Arena* out, type value) {                     \
        int len = strnlen(value, 65535);                           \
        AssertM(len < 65535, "printed string would be truncated"); \
        if (!out) {                                                \
            printf("%.*s", len, value);                            \
            return;                                                \
        }                                                          \
        Slice<char> out_buffer = out->push_many<char>(len);        \
        MemCopy(out_buffer.elems, value, len);                     \
    }
ImplPrintValueStr(char*);
ImplPrintValueStr(cchar*);
#undef ImplPrintValueStr

void print_value(Arena* out, bool value) {
    print_value(out, value ? "true" : "false");
}
void print_value(Arena* out, char value) {
    if (out) {
        *out->push<char>() = value;
    } else {
        printf("%c", value);
    }
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

void str_print(Arena* out) {}
forall(T, ... Args) Str str_print(Arena* out, T value, Args... args) {
    u8* arena_start = out->cur;
    print_value(out, value);
    str_print(out, args...);
    return Str{(char*)arena_start, (usize)(out->cur - arena_start)};
}
forall(... Args) Str str_println(Arena* out, Args... args) {
    u8* arena_start = out->cur;
    str_print(out, args...);
    str_print(out, "\n");
    return Str{(char*)arena_start, (usize)(out->cur - arena_start)};
}

void print_value(Arena* out, Str value) {
    if (!out) {
        printf("%.*s", (int)value.count, value.elems);
        return;
    }
    Slice<char> out_buf = out->push_many<char>(value.count);
    MemCopy(out_buf.elems, value.elems, value.count);
}

template <u8 CAPACITY>
void print_value(Arena* out, InlineStr<CAPACITY>* value) {
    print_value(out, value->to_str());
}

// -----------------------------------------------------------------------------

Str Str::from_cstr(cchar* cstr, usize maxlen) {
    return Str{cstr, strnlen(cstr, maxlen)};
}

Str Str::from_nullable_cstr(cchar* nullable_cstr, usize maxlen) {
    return nullable_cstr == nullptr ? Str{} : Str::from_cstr(nullable_cstr, maxlen);
}

Str Str::from_slice_char(Slice<char> slice) {
    return Str{(cchar*)slice.elems, slice.count};
}

Slice<char> Str::to_slice() {
    return Slice<char>{(char*)elems, count};
}

char* Str::to_cstr(Arena* arena) {
    char* ret = arena->push_many<char>(count + 1).elems;
    ArrayCopy(ret, elems, count);
    ret[count] = 0;
    return ret;
}

bool Str::eq(Str other) {
    if (count != other.count) return false;
    return strncmp(elems, other.elems, count) == 0;
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
    while (str.count > 0 && isspace(*str.elems)) {
        str.elems++;
        str.count--;
    }
    if (str.count == 0) return str;
    cchar* str_end = str.elems + str.count - 1;
    while (str.count > 0 && isspace(*str_end)) {
        str_end--;
        str.count--;
    }
    return str;
}

bool Str::starts_with(Str str) {
    int i = 0;
    for (; i < count && i < str.count; ++i) {
        if (elems[i] != str.elems[i]) return false;
    }
    return i < count || count == str.count;
}

bool Str::ends_with(Str str) {
    if (str.count > count) return false;
    int i = str.count - 1;
    int j = count - 1;
    for (; i >= 0; --i, --j) {
        if (elems[j] != str.elems[i]) return false;
    }
    return j >= 0 || count == str.count;
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

Str Str::substr_from_ptr(cchar* ptr) {
    Assert(ptr >= elems && ptr < elems + count);
    usize diff = (usize)(ptr - elems);
    return Str{ptr, count - diff};
}

Str Str::substr(usize idx, usize len) {
    return substr_from(idx).substr_to(len);
}

u64 Str::parse_u64(int base) {
    char buffer[22];
    ArrayCopy(buffer, elems, min(21ul, count));
    buffer[count] = 0;
    return strtoull(buffer, nullptr, base);
}

i64 Str::parse_i64(int base) {
    char buffer[22];
    ArrayCopy(buffer, elems, min(21ul, count));
    buffer[count] = 0;
    return strtoll(buffer, nullptr, base);
}

u32 Str::parse_u32(int base) {
    char buffer[12];
    ArrayCopy(buffer, elems, min(11ul, count));
    buffer[count] = 0;
    return strtoul(buffer, nullptr, base);
}

int Str::parse_int(int base) {
    char buffer[12];
    ArrayCopy(buffer, elems, min(11ul, count));
    buffer[count] = 0;
    return strtol(buffer, nullptr, base);
}

float Str::parse_float() {
    char buffer[32];
    ArrayCopy(buffer, elems, min(31ul, count));
    buffer[count] = 0;
    return strtof(buffer, nullptr);
}

double Str::parse_double() {
    char buffer[32];
    ArrayCopy(buffer, elems, min(31ul, count));
    buffer[count] = 0;
    return strtod(buffer, nullptr);
}

Str Str::join(Arena* out, Slice<Str> strings, char join) {
    if (strings.count == 0) {
        return Str{};
    }
    auto sb = StrBuilder::make(out);
    foreach (it, strings.iter()) {
        sb.print(*it.item);
        sb.print(join);
    }
    Str ret = sb.to_str();
    ret.count--;

    return ret;
}

Str Str::clone(Arena* arena) {
    Slice<char> cloned = arena->push_many<char>(count);
    ArrayCopy(cloned.elems, elems, count);
    return Str::from_slice_char(cloned);
}

Str Str::replace(Arena* out, Str match, Str newval) {
    if (match.count > count || match.count == 0) {
        return clone(out);
    }

    u8* result_start = out->cur;

    cchar* read = elems;
    cchar* end = elems + count;
    for (;;) {
        isize remaining = end - read;
        if (remaining < match.count) break;

        cchar* found = (cchar*)memchr(read, match.elems[0], remaining);
        if (!found) break;
        if (end - found < match.count) break;

        out->push_bytes((void*)read, found - read);

        Str candidate = Str{found, match.count};
        if (candidate.eq(match)) {
            out->push_bytes((void*)newval.elems, newval.count);
            read = found + match.count;
        } else {
            out->push_bytes((void*)found, 1);
            read = found + 1;
        }
    }

    out->push_bytes((void*)read, end - read);

    return Str{(char*)result_start, (usize)(out->cur - result_start)};
}

StrSplitCharIter Str::split_char_iter(char chr) {
    return StrSplitCharIter::make(*this, chr);
}

StrSplitWhitespaceIter Str::split_whitespace_iter() {
    return StrSplitWhitespaceIter::make(*this);
}

StrChunksIter Str::chunks_iter(usize size) {
    return StrChunksIter::make(*this, size);
}

// -----------------------------------------------------------------------------

StrSplitCharIter StrSplitCharIter::make(Str target, char chr) {
    StrSplitCharIter ret = {};
    ret.split = chr;
    ret.target = target;
    ret.item_end = target.elems - 1;
    ret.target_end = target.elems + target.count;
    ret.next();
    return ret;
}

void StrSplitCharIter::next() {
    Assert(!done);
    item_end++;
    item.elems = item_end;
    if (item.elems >= target_end) {
        done = true;
        return;
    }
    while (item_end < target_end && *item_end != split) {
        item_end++;
    }
    item.count = item_end - item.elems;
}

StrSplitWhitespaceIter StrSplitWhitespaceIter::make(Str target) {
    StrSplitWhitespaceIter ret = {};
    ret.target = target;
    ret.item_end = target.elems;
    ret.target_end = target.elems + target.count;
    ret.next();
    return ret;
}

void StrSplitWhitespaceIter::next() {
    Assert(!done);
    while (item_end < target_end && isspace(*item_end)) {
        item_end++;
    }
    item.elems = item_end;
    if (item.elems >= target_end) {
        done = true;
        return;
    }
    while (item_end < target_end && !isspace(*item_end)) {
        item_end++;
    }
    item.count = item_end - item.elems;
}

StrChunksIter StrChunksIter::make(Str target, usize size) {
    StrChunksIter ret = {};
    if (target.count < size) {
        ret.done = true;
        return ret;
    }
    ret.target_end = target.elems + target.count;
    ret.chunk_size = size;
    ret.item = Str{target.elems, size};
    return ret;
}

void StrChunksIter::next() {
    Assert(!done);
    if (item.elems + chunk_size >= target_end) {
        done = true;
    } else {
        item.elems += chunk_size;
    }
}

// -----------------------------------------------------------------------------

void print_value(Arena* out, U64PrintedWithCommas value) {
    char buffer[32];
    char out_buffer[32];
    int len = snprintf(buffer, sizeof(buffer), "%llu", value.val);
    int commas = (len - 1) / 3;
    char* write = out_buffer + len + commas;

    *write-- = '\0';

    for (int i = len - 1, c = 0; i >= 0; --i) {
        *write-- = buffer[i];
        if (++c == 3 && i != 0) {
            *write-- = ',';
            c = 0;
        }
    }

    print_value(out, out_buffer);
}

// -----------------------------------------------------------------------------
}  // namespace a
