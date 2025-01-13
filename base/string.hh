#pragma once
#include "inc.hh"
namespace {
// -----------------------------------------------------------------------------

void print_value(Vec<char>* out, cchar* value);
void print_value(Vec<char>* out, char* value);
void print_value(Vec<char>* out, char value);
void print_value(Vec<char>* out, i8 value);
void print_value(Vec<char>* out, u8 value);
void print_value(Vec<char>* out, i16 value);
void print_value(Vec<char>* out, u16 value);
void print_value(Vec<char>* out, i32 value);
void print_value(Vec<char>* out, u32 value);
void print_value(Vec<char>* out, i64 value);
void print_value(Vec<char>* out, u64 value);
void print_value(Vec<char>* out, usize value);
void print_value(Vec<char>* out, f32 value);
void print_value(Vec<char>* out, f64 value);
void print_value(Vec<char>* out, bool value);

void print();
forall(T, ... Args) void print(T value, Args... args);
forall(... Args) void println(Args... args);

void str_print(Vec<char>* out);
forall(T, ... Args) void str_print(Vec<char>* out, T value, Args... args);
forall(... Args) void str_println(Vec<char>* out, Args... args);

#if DEBUG
void x_log_print();
forall(T, ... Args) void x_log_print(T value, Args... args);
#define log(...)                                   \
    do {                                           \
        printf(">> %s:%i : ", __FILE__, __LINE__); \
        x_log_print(__VA_ARGS__);                  \
        printf("\n");                              \
    } while (0)
#else
#define log(...)
#endif

// -----------------------------------------------------------------------------

struct Str {
    cchar* elems;
    usize  count;

    static Str from_cstr(cchar* cstr);
    static Str from_nullable_cstr(cchar* nullable_cstr);
    char*      to_cstr(Arena* arena);
    bool       eq_cstr(cchar* cstr);
    bool       eq(Str other);

    Str clone(Arena* arena);

    Str before_first_index(char split);
    Str after_first_index(char split);
    Str after_last_index(char split);
    Str trim();

    bool starts_with_cstr(cchar* cstr);
    Str  substr_to(usize idx);
    Str  substr_from(usize idx);
    Str  substr(usize idx, usize len);

    u64 parse_u64(i32 base);
    i64 parse_i64(i32 base);
    u32 parse_u32(i32 base);
    i32 parse_i32(i32 base);
    f32 parse_f32();
    f64 parse_f64();
};
void print_value(Vec<char>* out, Str value);

#define StrLit(cstr_lit) Str{cstr_lit, sizeof(cstr_lit) - 1}

// -----------------------------------------------------------------------------

bool cstr_eq(cchar* a, cchar* b);

// -----------------------------------------------------------------------------

class StrSplitCharIter {
  private:
    bool   done;
    char   split;
    Str    target;
    Str    item;
    cchar* target_end_;
    cchar* item_end_;

  public:
    static StrSplitCharIter start(Str target, char chr) {
        StrSplitCharIter ret = {};

        ret.split       = chr;
        ret.target      = target;
        ret.item_end_   = target.elems - 1;
        ret.target_end_ = target.elems + target.count;

        ret.next();
        return ret;
    }
    void next() {
        item_end_++;
        item.elems = item_end_;
        if (item.elems >= target_end_) {
            done = true;
            return;
        }
        while (item_end_ < target_end_ && *item_end_ != split) {
            item_end_++;
        }
        item.count = item_end_ - item.elems;
    }
    Str               operator*() { return item; }
    bool              operator!=(StrSplitCharIter& other) { return !done; }
    StrSplitCharIter& operator++() { return next(), *this; }
};

struct StrSplitChar : NoCopy {
    Str  target;
    char chr;
    StrSplitChar(Str target, char chr) : target(target), chr(chr) {}
    StrSplitCharIter begin() { return StrSplitCharIter::start(target, chr); }
    StrSplitCharIter end() { return StrSplitCharIter{}; }
};

// -----------------------------------------------------------------------------

class StrSplitWhitespaceIter {
  private:
    bool   done;
    Str    target;
    Str    item;
    cchar* target_end_;
    cchar* item_end_;

  public:
    static StrSplitWhitespaceIter start(Str target) {
        StrSplitWhitespaceIter ret = {};

        ret.target      = target;
        ret.item_end_   = target.elems;
        ret.target_end_ = target.elems + target.count;

        ret.next();
        return ret;
    }
    void next() {
        while (item_end_ < target_end_ && isspace(*item_end_)) {
            item_end_++;
        }
        item.elems = item_end_;
        if (item.elems >= target_end_) {
            done = true;
            return;
        }
        while (item_end_ < target_end_ && !isspace(*item_end_)) {
            item_end_++;
        }
        item.count = item_end_ - item.elems;
    }
    Str                     operator*() { return item; }
    bool                    operator!=(StrSplitWhitespaceIter& other) { return !done; }
    StrSplitWhitespaceIter& operator++() { return next(), *this; }
};

struct StrSplitWhitespace : NoCopy {
    Str target;
    StrSplitWhitespace(Str target) : target(target) {}
    StrSplitWhitespaceIter begin() { return StrSplitWhitespaceIter::start(target); }
    StrSplitWhitespaceIter end() { return StrSplitWhitespaceIter{}; }
};

// -----------------------------------------------------------------------------

struct U64PrintedWithCommas {
    u64 val;
    U64PrintedWithCommas(u64 val) : val(val) {}
};
void print_value(Vec<char>* out, U64PrintedWithCommas value);

// -----------------------------------------------------------------------------

}  // namespace