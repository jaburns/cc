#pragma once
#include "inc.hh"
namespace a {
// -----------------------------------------------------------------------------

struct vec2;
struct vec3;
struct vec3a;
struct vec4;
struct ivec2;
struct ivec3;
struct ivec4;
struct uvec2;
struct mat2;
struct mat4;

// -----------------------------------------------------------------------------

struct vec2 {
    union {
        struct {
            float x, y;
        };
        f32x2 vector;
    };

    func vec2 splat(float val);

    vec2 min(vec2 rhs);
    vec2 max(vec2 rhs);
    vec2 add_scaled(float scale, vec2 rhs);
    vec2 lerp(vec2 targ, float t);
    float dot(vec2 rhs);
    float cross(vec2 rhs);
    vec2 perp();
    vec2 fract();
    float length();
    float length_sqr();
    float distance(vec2 targ);
    vec2 abs();
    vec2 sign();
    vec2 clamp(vec2 min, vec2 max);
    vec2 normalize();
    vec2 rotate(float radians);
    vec2 reflect_and_scale(vec2 normal, float norm_scale, float tan_scale);

    vec3a to_vec3a(float z);
    ivec2 floor_to_ivec2();
    ivec2 ceil_to_ivec2();
    ivec2 round_to_ivec2();
};

vec2 operator+(vec2 lhs, vec2 rhs);
vec2& operator+=(vec2& lhs, vec2 rhs);
vec2 operator-(vec2 lhs, vec2 rhs);
vec2 operator-(vec2 vec);
vec2& operator-=(vec2& lhs, vec2 rhs);
vec2 operator*(vec2 lhs, float rhs);
vec2 operator*(float lhs, vec2 rhs);
vec2 operator*(vec2 lhs, vec2 rhs);
vec2& operator*=(vec2& lhs, float rhs);
vec2 operator/(vec2 lhs, float rhs);
vec2& operator/=(vec2& lhs, float rhs);

#define vec2(x_, y_) (vec2{.vector = {(float)(x_), (float)(y_)}})
#define vec2_from_f32x2(v_) (vec2{.vector = (v_)})

#define VEC2_ZERO (vec2(0, 0))
#define VEC2_ONE (vec2(1, 1))
#define VEC2_HALF (vec2(.5, .5))
#define VEC2_RIGHT (vec2(1, 0))
#define VEC2_UP (vec2(0, 1))
#define VEC2_LEFT (vec2(-1, 0))
#define VEC2_DOWN (vec2(0, -1))

// -----------------------------------------------------------------------------

struct ivec2 {
    union {
        struct {
            int x, y;
        };
        i32x2 vector;
    };

    func ivec2 splat(int val);

    int manhattan();
    ivec2 abs();
    ivec2 min(ivec2 rhs);
    ivec2 max(ivec2 rhs);
    ivec2 clamp(ivec2 min_inclusive, ivec2 max_exclusive);

    vec2 to_vec2();
    uvec2 to_uvec2_saturate();
};

ivec2 operator+(ivec2 lhs, ivec2 rhs);
ivec2& operator+=(ivec2& lhs, ivec2 rhs);
ivec2 operator-(ivec2 lhs, ivec2 rhs);
ivec2 operator-(ivec2 vec);
ivec2& operator-=(ivec2& lhs, ivec2 rhs);
ivec2 operator*(ivec2 lhs, int rhs);
ivec2 operator*(int lhs, ivec2 rhs);
ivec2 operator*(ivec2 lhs, ivec2 rhs);
ivec2& operator*=(ivec2& lhs, int rhs);
bool operator==(ivec2 lhs, ivec2 rhs);

void print_value(Arena* out, ivec2 v);

#define ivec2(x_, y_) (ivec2{.vector = {(int)(x_), (int)(y_)}})
#define ivec2_from_i32x2(v_) (ivec2{.vector = (v_)})

#define IVEC2_ZERO (ivec2(0, 0))
#define IVEC2_ONE (ivec2(1, 1))
#define IVEC2_RIGHT (ivec2(1, 0))
#define IVEC2_UP (ivec2(0, 1))
#define IVEC2_LEFT (ivec2(-1, 0))
#define IVEC2_DOWN (ivec2(0, -1))

// -----------------------------------------------------------------------------

struct uvec2 {
    union {
        struct {
            u32 x, y;
        };
        u32x2 vector;
    };

    uvec2 min(uvec2 rhs);
    uvec2 max(uvec2 rhs);

    vec2 to_vec2();
    ivec2 to_ivec2();
};

uvec2 operator+(uvec2 lhs, uvec2 rhs);
uvec2& operator+=(uvec2& lhs, uvec2 rhs);
uvec2 operator-(uvec2 lhs, uvec2 rhs);
uvec2& operator-=(uvec2& lhs, uvec2 rhs);
uvec2 operator*(uvec2 lhs, u32 rhs);
uvec2 operator*(u32 lhs, uvec2 rhs);
uvec2 operator*(uvec2 lhs, uvec2 rhs);
uvec2& operator*=(uvec2& lhs, u32 rhs);
bool operator==(uvec2 lhs, uvec2 rhs);

void print_value(Arena* out, uvec2 v);

#define uvec2(x_, y_) (uvec2{.vector = {(u32)(x_), (u32)(y_)}})
#define uvec2_from_u32x2(v_) (uvec2{.vector = (v_)})

#define UVEC2_ZERO (uvec2(0, 0))
#define UVEC2_ONE (uvec2(1, 1))

// -----------------------------------------------------------------------------

struct vec3 {
    float x, y, z;

    vec3a to_vec3a();
};
struct vec3a {
    union {
        struct {
            float x, y, z, zero;
        };
        f32x4 vector;
    };

    float dot(vec3a rhs);
    vec3a cross(vec3a rhs);
    vec3a lerp(vec3a targ, float t);
    vec3a normalize();

    vec3 to_vec3();
    vec4 to_vec4(float w);
};

vec3a operator+(vec3a lhs, vec3a rhs);
vec3a& operator+=(vec3a& lhs, vec3a rhs);
vec3a operator-(vec3a lhs, vec3a rhs);
vec3a operator-(vec3a vec);
vec3a& operator-=(vec3a& lhs, vec3a rhs);
vec3a operator*(vec3a lhs, float rhs);
vec3a operator*(float lhs, vec3a rhs);
vec3a operator*(vec3a lhs, vec3a rhs);
vec3a& operator*=(vec3a& lhs, float rhs);
vec3a operator/(vec3a lhs, float rhs);
vec3a& operator/=(vec3a& lhs, float rhs);

void print_value(Arena* out, vec3 v);
void print_value(Arena* out, vec3a v);

#define vec3(x_, y_, z_) (vec3{(float)(x_), (float)(y_), (float)(z_)})
#define vec3a(x_, y_, z_) (vec3a{.vector = {(float)(x_), (float)(y_), (float)(z_), 0.f}})
#define vec3a_from_f32x4(v_) (vec3a{.vector = (v_)})

#define VEC3A_ZERO (vec3a(0, 0, 0))
#define VEC3A_ONE (vec3a(1, 1, 1))
#define VEC3A_LEFT (vec3a(-1, 0, 0))
#define VEC3A_RIGHT (vec3a(1, 0, 0))
#define VEC3A_BACK (vec3a(0, -1, 0))
#define VEC3A_FORWARD (vec3a(0, 1, 0))
#define VEC3A_UP (vec3a(0, 0, 1))
#define VEC3A_DOWN (vec3a(0, 0, -1))

// -----------------------------------------------------------------------------

struct ivec3 {
    int x, y, z;
};

#define ivec3(x_, y_, z_) (ivec3{(int)(x_), (int)(y_), (int)(z_)})

// -----------------------------------------------------------------------------

struct uvec3 {
    u32 x, y, z;
};

#define uvec3(x_, y_, z_) (uvec3{(u32)(x_), (u32)(y_), (u32)(z_)})

// -----------------------------------------------------------------------------

struct vec4 {
    union {
        struct {
            float x, y, z, w;
        };
        struct {
            float r, g, b, a;
        };
        f32x4 vector;
    };

    float dot(vec4 rhs);

    vec3a xyz() { return vec3a(x, y, z); }
};

vec4 operator+(vec4 lhs, vec4 rhs);
vec4& operator+=(vec4& lhs, vec4 rhs);
vec4 operator-(vec4 lhs, vec4 rhs);
vec4 operator-(vec4 vec);
vec4& operator-=(vec4& lhs, vec4 rhs);
vec4 operator*(vec4 lhs, float rhs);
vec4 operator*(float lhs, vec4 rhs);
vec4 operator*(vec4 lhs, vec4 rhs);
vec4& operator*=(vec4& lhs, float rhs);
vec4 operator/(vec4 lhs, float rhs);
vec4& operator/=(vec4& lhs, float rhs);

void print_value(Arena* out, vec4 v);

#define vec4(x_, y_, z_, w_) (vec4{.vector = {(float)(x_), (float)(y_), (float)(z_), (float)(w_)}})
#define vec4_from_f32x4(v_) (vec4{.vector = (v_)})

#define VEC4_ZERO (vec4(0, 0, 0, 0))
#define VEC4_BLACK (vec4(0, 0, 0, 1))
#define VEC4_RED (vec4(1, 0, 0, 1))
#define VEC4_GREEN (vec4(0, 1, 0, 1))
#define VEC4_BLUE (vec4(0, 0, 1, 1))
#define VEC4_CYAN (vec4(0, 1, 1, 1))
#define VEC4_MAGENTA (vec4(1, 0, 1, 1))
#define VEC4_YELLOW (vec4(1, 1, 0, 1))
#define VEC4_WHITE (vec4(1, 1, 1, 1))

// -----------------------------------------------------------------------------

struct ivec4 {
    union {
        struct {
            int x, y, z, w;
        };
        i32x4 vector;
    };
};

#define ivec4(x_, y_, z_, w_) (ivec4{.vector = {(int)(x_), (int)(y_), (int)(z_), (int)(w_)}})
#define ivec4_from_i32x4(v_) (ivec4{.vector = (v_)})

// -----------------------------------------------------------------------------

struct uvec4 {
    union {
        struct {
            u32 x, y, z, w;
        };
        u32x4 vector;
    };
};

#define uvec4(x_, y_, z_, w_) (uvec4{.vector = {(u32)(x_), (u32)(y_), (u32)(z_), (u32)(w_)}})
#define uvec4_from_u32x4(v_) (uvec4{.vector = (v_)})

// -----------------------------------------------------------------------------

struct mat2 {
    vec2 a, b;

    mat2& mk_identity();
    mat2& mk_rotation(float radians);
    mat2& mk_rotation_pi_over_2();
    mat2& mk_rotation_pi();
};

vec2 operator*(mat2& lhs, vec2 rhs);

void print_value(Arena* out, mat2 v);

// -----------------------------------------------------------------------------

struct mat4 {
    vec4 a, b, c, d;

    mat4& mk_identity();
    mat4& mk_ortho(float left, float right, float bottom, float top, float z_near, float z_far);
    mat4& mk_perspective(float fov_y, float aspect, float z_near, float z_far);
    mat4& mk_rotation_angle_axis(float angle, vec3a normalized_axis);
    mat4& mk_look_at(vec3a eye, vec3a target, vec3a up);
    mat4& mk_view(vec3a camera_pos, vec3a camera_fwd_normalized, vec3a camera_up_normalized);

    mat4& apply_scale(vec3a scale);
    mat4& apply_translation(vec3a translate);

    vec4 mul_vec4(vec4 rhs);
};

mat4 operator*(mat4 lhs, mat4 rhs);
mat4& operator*=(mat4& lhs, mat4 rhs);

void print_value(Arena* out, mat4 v);

// -----------------------------------------------------------------------------

#define sincosf __sincosf

float f32_lerp(float a, float b, float t);
float f32_fract(float a);
i64 i64_mod(i64 a, i64 b);

float radians_sub(float lhs, float rhs);
float radians_lerp(float a, float b, float t);
float radians_mod_zero_to_2pi(float rads);
float radians_minimize_unknown_sine(float (*f)(float));

// -----------------------------------------------------------------------------

float ease_out_back(float x);
float ease_out_quad(float x);
float ease_in_quad(float x);
float ease_out_cubic(float x);
float ease_in_cubic(float x);
float ease_in_out_quad(float x);

// -----------------------------------------------------------------------------
namespace geo {

class BresenhamIter {
    ivec2 p1;
    ivec2 d, s;
    i32 err;

  public:
    bool done;
    ivec2 item;

    func BresenhamIter make(ivec2 p0, ivec2 p1);
    void next();
};

struct LineSegPointResult {
    bool on_line;
    vec2 pt;
};

struct LineSegIntersectResult {
    bool hit;
    vec2 point;
    vec2 normal;
    float t;
};

LineSegPointResult closest_point_on_line_seg(vec2 pt, vec2 line0, vec2 line1);
bool rect_overlaps_circle(vec2 min, vec2 max, vec2 circle_center, float circle_radius);
bool rect_overlaps_rect(vec2 min0, vec2 max0, vec2 min1, vec2 max1);
LineSegIntersectResult line_hit_oriented_line(vec2 a0, vec2 a1, vec2 b0, vec2 b1);
LineSegIntersectResult line_hit_circle(vec2 p0, vec2 p1, vec2 center, float radius);
LineSegIntersectResult line_hit_rect(vec2 p0, vec2 p1, vec2 rect_min, vec2 rect_max);
bool rect_contains_point(vec2 rect_min, vec2 rect_max, vec2 pt);

}  // namespace geo
// -----------------------------------------------------------------------------
namespace random {

int range(int min_inclusive, int max_exclusive);
float value();

}  // namespace random
// -----------------------------------------------------------------------------

forall(T) void flip_grid_subset_x(T* elems, uvec2 grid_size, uvec2 subset_pos, uvec2 subset_size);
forall(T) void flip_grid_subset_y(T* elems, uvec2 grid_size, uvec2 subset_pos, uvec2 subset_size);

// -----------------------------------------------------------------------------
}  // namespace a
