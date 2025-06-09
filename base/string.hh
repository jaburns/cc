#pragma once
#include "inc.hh"
namespace a {
// -----------------------------------------------------------------------------

void print_value(Arena* out, cchar* value);
void print_value(Arena* out, char* value);
void print_value(Arena* out, char value);
void print_value(Arena* out, i8 value);
void print_value(Arena* out, u8 value);
void print_value(Arena* out, i16 value);
void print_value(Arena* out, u16 value);
void print_value(Arena* out, i32 value);
void print_value(Arena* out, u32 value);
void print_value(Arena* out, i64 value);
void print_value(Arena* out, u64 value);
void print_value(Arena* out, usize value);
void print_value(Arena* out, isize value);
void print_value(Arena* out, float value);
void print_value(Arena* out, double value);
void print_value(Arena* out, bool value);

void print();
forall(T, ... Args) void print(T value, Args... args);
forall(... Args) void println(Args... args);

#if DEBUG
void x_log_print();
forall(T, ... Args) void x_log_print(T value, Args... args);
#define log(...)                                                      \
    do {                                                              \
        u64 ms = timing_ticks_to_nanos(timing_get_ticks()) / 1000000; \
        printf(">> [%llu] %s:%i : ", ms, __FILE__, __LINE__);         \
        x_log_print(__VA_ARGS__);                                     \
        printf("\n");                                                 \
    } while (0)
#else
#define log(...)
#endif

// -----------------------------------------------------------------------------

class StrSplitCharIter;
class StrSplitWhitespaceIter;
class StrChunksIter;

struct Str {
    cchar* elems;
    usize count;
    // ---

    template <usize N>
    constexpr Str(cchar (&arr)[N]) : elems(arr), count(N - 1) {}
    constexpr Str(cchar* p, usize n) : elems(p), count(n) {}
    constexpr Str() = default;

    konst usize DEFAULT_MAX_CSTR_LEN = 4096;

    func Str from_cstr(cchar* cstr, usize maxlen = DEFAULT_MAX_CSTR_LEN);
    func Str from_nullable_cstr(cchar* nullable_cstr, usize maxlen = DEFAULT_MAX_CSTR_LEN);
    func Str from_slice_char(Slice<char> slice);
    Slice<char> to_slice();
    char* to_cstr(Arena* arena);
    bool eq(Str other);

    Str before_first_index(char split);
    Str after_first_index(char split);
    Str after_last_index(char split);
    Str trim();

    bool starts_with(Str str);
    bool ends_with(Str str);
    Str substr_to(usize idx);
    Str substr_from(usize idx);
    Str substr_from_ptr(cchar* ptr);
    Str substr(usize idx, usize len);

    u64 parse_u64(int base);
    i64 parse_i64(int base);
    u32 parse_u32(int base);
    int parse_int(int base);
    float parse_float();
    double parse_double();

    Str clone(Arena* arena);
    func Str join(Arena* out, Slice<Str> strings, char join);
    Str replace(Arena* out, Str match, Str newval);

    StrSplitCharIter split_char_iter(char chr);
    StrSplitWhitespaceIter split_whitespace_iter();

    // gives chunks as long as they fit, does not return the remainder
    StrChunksIter chunks_iter(usize size);
};

void print_value(Arena* out, Str value);

#define StaticStrLen(cstr) (sizeof(cstr) - 1)

// -----------------------------------------------------------------------------

void str_print(Arena* out);
forall(T, ... Args) Str str_print(Arena* out, T value, Args... args);
forall(... Args) Str str_println(Arena* out, Args... args);

// -----------------------------------------------------------------------------

template <u8 CAPACITY>
struct InlineStr {
    konst u8 capacity = CAPACITY;

    u8 count;
    char elems[CAPACITY];

  public:
    Str to_str() { return Str{elems, count}; }
    void set(Str str) {
        Assert(str.count <= CAPACITY);
        ZeroStruct(this);
        count = str.count;
        ArrayCopy(elems, str.elems, str.count);
    }
};

template <u8 CAPACITY>
void print_value(Arena* out, InlineStr<CAPACITY>* value);

// -----------------------------------------------------------------------------

struct StrBuilder {
    u8* arena_start;
    Arena* arena;

    func StrBuilder make(Arena* arena) {
        StrBuilder ret = {};
        ret.arena = arena;
        ret.arena_start = arena->cur;
        return ret;
    }

    forall(... Args) void print(Args... args) { str_print(arena, args...); }
    forall(... Args) void println(Args... args) { str_println(arena, args...); }
    Str to_str() { return Str{(char*)arena_start, (usize)(arena->cur - arena_start)}; }
};

// -----------------------------------------------------------------------------

class StrSplitCharIter {
  private:
    char split;
    Str target;
    cchar* target_end;
    cchar* item_end;

  public:
    bool done;
    Str item;
    func StrSplitCharIter make(Str target, char chr);
    void next();
};

class StrSplitWhitespaceIter {
  private:
    Str target;
    cchar* target_end;
    cchar* item_end;

  public:
    bool done;
    Str item;
    func StrSplitWhitespaceIter make(Str target);
    void next();
};

class StrChunksIter {
  private:
    cchar* target_end;
    usize chunk_size;

  public:
    bool done;
    Str item;
    func StrChunksIter make(Str target, usize size);
    void next();
};

// -----------------------------------------------------------------------------

bool cstr_eq(cchar* a, cchar* b) { return strcmp(a, b) == 0; }

// -----------------------------------------------------------------------------

struct U64PrintedWithCommas {
    u64 val;
    U64PrintedWithCommas(u64 val) : val(val) {}
};
void print_value(Arena* out, U64PrintedWithCommas value);

// -----------------------------------------------------------------------------
}  // namespace a
