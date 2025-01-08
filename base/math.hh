#pragma once
#include "inc.hh"
namespace {
// -----------------------------------------------------------------------------

struct vec2;
struct vec3;
struct vec3a;
struct vec4;
struct ivec2;
struct ivec3;
struct ivec4;
struct mat2;
struct mat4;

// -----------------------------------------------------------------------------

struct vec2 {
    union {
        struct {
            f32 x, y;
        };
        f32x2 vector;
    };

    static vec2 splat(f32 val);

    vec2 min(vec2 rhs);
    vec2 max(vec2 rhs);
    vec2 add_scaled(f32 scale, vec2 rhs);
    vec2 lerp(vec2 targ, f32 t);
    f32  dot(vec2 rhs);
    f32  cross(vec2 rhs);
    vec2 perp();
    vec2 fract();
    f32  length();
    f32  length_sqr();
    f32  distance(vec2 targ);
    vec2 abs();
    vec2 sign();
    vec2 clamp(vec2 min, vec2 max);
    vec2 normalize();
    vec2 rotate(f32 radians);
    vec2 reflect_and_scale(vec2 normal, f32 norm_scale, f32 tan_scale);

    vec3a to_vec3a(f32 z);
    ivec2 floor_to_ivec2();
    ivec2 ceil_to_ivec2();
    ivec2 round_to_ivec2();
};

vec2  operator+(vec2 lhs, vec2 rhs);
vec2& operator+=(vec2& lhs, vec2 rhs);
vec2  operator-(vec2 lhs, vec2 rhs);
vec2  operator-(vec2 vec);
vec2& operator-=(vec2& lhs, vec2 rhs);
vec2  operator*(vec2 lhs, f32 rhs);
vec2  operator*(f32 lhs, vec2 rhs);
vec2  operator*(vec2 lhs, vec2 rhs);
vec2& operator*=(vec2& lhs, f32 rhs);
vec2  operator/(vec2 lhs, f32 rhs);
vec2& operator/=(vec2& lhs, f32 rhs);

void print_value(Vec<char>* out, vec2 v);

#define vec2(x_, y_)        (vec2{.vector = {(f32)(x_), (f32)(y_)}})
#define vec2_from_f32x2(v_) (vec2{.vector = (v_)})

#define VEC2_ZERO  (vec2(0, 0))
#define VEC2_ONE   (vec2(1, 1))
#define VEC2_HALF  (vec2(.5, .5))
#define VEC2_RIGHT (vec2(1, 0))
#define VEC2_UP    (vec2(0, 1))
#define VEC2_LEFT  (vec2(-1, 0))
#define VEC2_DOWN  (vec2(0, -1))

// -----------------------------------------------------------------------------

struct ivec2 {
    union {
        struct {
            i32 x, y;
        };
        i32x2 vector;
    };

    static ivec2 splat(i32 val);

    i32   manhattan();
    ivec2 abs();
    ivec2 min(ivec2 rhs);
    ivec2 max(ivec2 rhs);
    bool  eq(ivec2 rhs);
    ivec2 clamp(ivec2 min_inclusive, ivec2 max_exclusive);

    vec2 to_vec2();
};

ivec2  operator+(ivec2 lhs, ivec2 rhs);
ivec2& operator+=(ivec2& lhs, ivec2 rhs);
ivec2  operator-(ivec2 lhs, ivec2 rhs);
ivec2  operator-(ivec2 vec);
ivec2& operator-=(ivec2& lhs, ivec2 rhs);
ivec2  operator*(ivec2 lhs, i32 rhs);
ivec2  operator*(i32 lhs, ivec2 rhs);
ivec2  operator*(ivec2 lhs, ivec2 rhs);
ivec2& operator*=(ivec2& lhs, i32 rhs);

void print_value(Vec<char>* out, vec2 v);

#define ivec2(x_, y_)        (ivec2{.vector = {(i32)(x_), (i32)(y_)}})
#define ivec2_from_i32x2(v_) (ivec2{.vector = (v_)})

#define IVEC2_ZERO  (ivec2(0, 0))
#define IVEC2_ONE   (ivec2(1, 1))
#define IVEC2_RIGHT (ivec2(1, 0))
#define IVEC2_UP    (ivec2(0, 1))
#define IVEC2_LEFT  (ivec2(-1, 0))
#define IVEC2_DOWN  (ivec2(0, -1))

// -----------------------------------------------------------------------------

struct vec3 {
    f32 x, y, z;

    vec3a to_vec3a();
};
struct vec3a {
    union {
        struct {
            f32 x, y, z, zero;
        };
        f32x4 vector;
    };

    f32   dot(vec3a rhs);
    vec3a cross(vec3a rhs);
    vec3a normalize();

    vec3 to_vec3();
    vec4 to_vec4(f32 w);
};

vec3a  operator+(vec3a lhs, vec3a rhs);
vec3a& operator+=(vec3a& lhs, vec3a rhs);
vec3a  operator-(vec3a lhs, vec3a rhs);
vec3a  operator-(vec3a vec);
vec3a& operator-=(vec3a& lhs, vec3a rhs);
vec3a  operator*(vec3a lhs, f32 rhs);
vec3a  operator*(f32 lhs, vec3a rhs);
vec3a  operator*(vec3a lhs, vec3a rhs);
vec3a& operator*=(vec3a& lhs, f32 rhs);
vec3a  operator/(vec3a lhs, f32 rhs);
vec3a& operator/=(vec3a& lhs, f32 rhs);

void print_value(Vec<char>* out, vec3 v);
void print_value(Vec<char>* out, vec3a v);

#define vec3(x_, y_, z_)     (vec3{(f32)(x_), (f32)(y_), (f32)(z_)})
#define vec3a(x_, y_, z_)    (vec3a{.vector = {(f32)(x_), (f32)(y_), (f32)(z_), 0.f}})
#define vec3a_from_f32x4(v_) (vec3a{.vector = (v_)})

#define VEC3A_ZERO    (vec3a(0, 0, 0))
#define VEC3A_ONE     (vec3a(1, 1, 1))
#define VEC3A_LEFT    (vec3a(-1, 0, 0))
#define VEC3A_RIGHT   (vec3a(1, 0, 0))
#define VEC3A_BACK    (vec3a(0, -1, 0))
#define VEC3A_FORWARD (vec3a(0, 1, 0))
#define VEC3A_UP      (vec3a(0, 0, 1))
#define VEC3A_DOWN    (vec3a(0, 0, -1))

// -----------------------------------------------------------------------------

struct ivec3 {
    i32 x, y, z;
};

#define ivec3(x_, y_, z_) (ivec3{(i32)(x_), (i32)(y_), (i32)(z_)})

// -----------------------------------------------------------------------------

struct vec4 {
    union {
        struct {
            f32 x, y, z, w;
        };
        struct {
            f32 r, g, b, a;
        };
        f32x4 vector;
    };
};

vec4  operator+(vec4 lhs, vec4 rhs);
vec4& operator+=(vec4& lhs, vec4 rhs);
vec4  operator-(vec4 lhs, vec4 rhs);
vec4  operator-(vec4 vec);
vec4& operator-=(vec4& lhs, vec4 rhs);
vec4  operator*(vec4 lhs, f32 rhs);
vec4  operator*(f32 lhs, vec4 rhs);
vec4  operator*(vec4 lhs, vec4 rhs);
vec4& operator*=(vec4& lhs, f32 rhs);
vec4  operator/(vec4 lhs, f32 rhs);
vec4& operator/=(vec4& lhs, f32 rhs);

void print_value(Vec<char>* out, vec4 v);

#define vec4(x_, y_, z_, w_) (vec4{.vector = {(f32)(x_), (f32)(y_), (f32)(z_), (f32)(w_)}})
#define vec4_from_f32x4(v_)  (vec4{.vector = (v_)})

#define VEC4_ZERO    (vec4(0, 0, 0, 0))
#define VEC4_BLACK   (vec4(0, 0, 0, 1))
#define VEC4_RED     (vec4(1, 0, 0, 1))
#define VEC4_GREEN   (vec4(0, 1, 0, 1))
#define VEC4_BLUE    (vec4(0, 0, 1, 1))
#define VEC4_CYAN    (vec4(0, 1, 1, 1))
#define VEC4_MAGENTA (vec4(1, 0, 1, 1))
#define VEC4_YELLOW  (vec4(1, 1, 0, 1))
#define VEC4_WHITE   (vec4(1, 1, 1, 1))

// -----------------------------------------------------------------------------

struct ivec4 {
    union {
        struct {
            i32 x, y, z, w;
        };
        i32x4 vector;
    };
};

#define ivec4(x_, y_, z_, w_) (ivec4{.vector = {(i32)(x_), (i32)(y_), (i32)(z_), (i32)(w_)}})

// -----------------------------------------------------------------------------

struct mat2 {
    vec2 a, b;

    mat2& mk_identity();
    mat2& mk_rotation(f32 radians);
    mat2& mk_rotation_pi_over_2();
    mat2& mk_rotation_pi();
};

vec2 operator*(mat2& lhs, vec2 rhs);

void print_value(Vec<char>* out, mat2 v);

// -----------------------------------------------------------------------------

struct mat4 {
    vec4 a, b, c, d;

    mat4& mk_identity();
    mat4& mk_ortho(f32 left, f32 right, f32 bottom, f32 top, f32 z_near, f32 z_far);
    mat4& mk_perspective(f32 fov_y, f32 aspect, f32 z_near, f32 z_far);
    mat4& mk_rotation_angle_axis(f32 angle, vec3a normalized_axis);
    mat4& mk_look_at(vec3a eye, vec3a target, vec3a up);
    mat4& mk_view(vec3a camera_pos, vec3a camera_fwd_normalized, vec3a camera_up_normalized);

    mat4& apply_scale(vec3a scale);
    mat4& apply_translation(vec3a translate);
};

mat4  operator*(mat4 lhs, mat4 rhs);
mat4& operator*=(mat4& lhs, mat4 rhs);

void print_value(Vec<char>* out, mat4 v);

// -----------------------------------------------------------------------------

#define sincosf __sincosf

f32 f32_lerp(f32 a, f32 b, f32 t);
f32 f32_fract(f32 a);
i64 i64_mod(i64 a, i64 b);

f32 radians_sub(f32 lhs, f32 rhs);
f32 radians_lerp(f32 a, f32 b, f32 t);
f32 radians_mod_zero_to_2pi(f32 rads);
f32 radians_minimize_unknown_sine(f32 (*f)(f32));

// -----------------------------------------------------------------------------

f32 ease_out_back(f32 x);
f32 ease_out_quad(f32 x);
f32 ease_in_quad(f32 x);
f32 ease_out_cubic(f32 x);
f32 ease_in_cubic(f32 x);
f32 ease_in_out_quad(f32 x);

// -----------------------------------------------------------------------------

struct LineSegPointResult {
    bool on_line;
    vec2 pt;
};

struct LineSegIntersectResult {
    bool hit;
    vec2 point;
    vec2 normal;
    f32  t;
};

LineSegPointResult     geo_closest_point_on_line_seg(vec2 pt, vec2 line0, vec2 line1);
bool                   geo_rect_overlaps_circle(vec2 min, vec2 max, vec2 circle_center, f32 circle_radius);
bool                   geo_rect_overlaps_rect(vec2 min0, vec2 max0, vec2 min1, vec2 max1);
LineSegIntersectResult geo_line_hit_oriented_line(vec2 a0, vec2 a1, vec2 b0, vec2 b1);
LineSegIntersectResult geo_line_hit_circle(vec2 p0, vec2 p1, vec2 center, f32 radius);
LineSegIntersectResult geo_line_hit_rect(vec2 p0, vec2 p1, vec2 rect_min, vec2 rect_max);
bool                   geo_rect_contains_point(vec2 rect_min, vec2 rect_max, vec2 pt);

// -----------------------------------------------------------------------------
}  // namespace