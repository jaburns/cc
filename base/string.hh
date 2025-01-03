#pragma once
#include "inc.hh"
namespace {

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

struct Str {
    cchar* elems;
    usize  count;

    static Str from_ptr(cchar* elems, usize count);
    static Str from_cstr(cchar* cstr);
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
};
void print_value(Vec<char>* out, Str value);

bool cstr_eq(cchar* a, cchar* b);

struct U64PrintedWithCommas {
    u64 val;
};
void print_value(Vec<char>* out, U64PrintedWithCommas value);

}  // namespace