#include "inc.hh"
namespace a {
// -----------------------------------------------------------------------------
// vec2

vec2 vec2::splat(float val) {
    return vec2_from_f32x2(f32x2_splat(val));
}

vec2 vec2::min(vec2 rhs) {
    return vec2_from_f32x2(f32x2_min(vector, rhs.vector));
}
vec2 vec2::max(vec2 rhs) {
    return vec2_from_f32x2(f32x2_max(vector, rhs.vector));
}
vec2 vec2::add_scaled(float scale, vec2 rhs) {
    return vec2_from_f32x2(f32x2_scale_add(vector, rhs.vector, scale));
}
vec2 vec2::lerp(vec2 targ, float t) {
    return vec2_from_f32x2(f32x2_scale_add(vector, f32x2_sub(targ.vector, vector), t));
}
float vec2::dot(vec2 rhs) {
    return f32x2_add_across(f32x2_mul(vector, rhs.vector));
}
float vec2::cross(vec2 rhs) {
    u32x2 sign_bit = {0, 0x80000000};
    f32x2 prod = f32x2_mul(vector, f32x2_reverse64(rhs.vector));
    f32x2 flipped = (f32x2)u32x2_xor((u32x2)prod, sign_bit);
    return f32x2_add_across(flipped);
}
vec2 vec2::perp() {
    u32x2 sign_bit = {0x80000000, 0};
    u32x2 bit_result = u32x2_xor((u32x2)f32x2_reverse64(vector), sign_bit);
    return vec2_from_f32x2((f32x2)bit_result);
}
vec2 vec2::fract() {
    return vec2_from_f32x2(f32x2_sub(vector, f32x2_floor(vector)));
}
float vec2::length() {
    return sqrtf(f32x2_add_across(f32x2_mul(vector, vector)));
}
float vec2::length_sqr() {
    return f32x2_add_across(f32x2_mul(vector, vector));
}
float vec2::distance(vec2 target) {
    f32x2 delta = f32x2_sub(vector, target.vector);
    return sqrtf(f32x2_add_across(f32x2_mul(delta, delta)));
}
vec2 vec2::abs() {
    return vec2_from_f32x2(f32x2_abs(vector));
}
vec2 vec2::sign() {
    u32x2 neg_mask = f32x2_less_than(vector, f32x2_splat(0.f));
    return vec2_from_f32x2(f32x2_select(neg_mask, f32x2_splat(-1.f), f32x2_splat(1.f)));
}
vec2 vec2::clamp(vec2 min, vec2 max) {
    return vec2_from_f32x2(f32x2_max(min.vector, f32x2_min(max.vector, vector)));
}
vec2 vec2::normalize() {
    float len = sqrtf(f32x2_add_across(f32x2_mul(vector, vector)));
    return len == 0.f ? VEC2_ZERO : vec2_from_f32x2(f32x2_div(vector, f32x2_splat(len)));
}
vec2 vec2::rotate(float radians) {
    float s, c;
    sincosf(radians, &s, &c);
    f32x2 rx = f32x2_mul(vector, f32x2{c, -s});
    f32x2 ry = f32x2_mul(vector, f32x2{s, c});
    return vec2_from_f32x2(f32x2_add_pairs(rx, ry));
}

vec3a vec2::to_vec3a(float z) {
    return vec3a(x, y, z);
}
ivec2 vec2::floor_to_ivec2() {
    return ivec2_from_i32x2(i32x2_from_f32x2(f32x2_floor(vector)));
}
ivec2 vec2::ceil_to_ivec2() {
    return ivec2_from_i32x2(i32x2_from_f32x2(f32x2_ceil(vector)));
}
ivec2 vec2::round_to_ivec2() {
    return ivec2_from_i32x2(i32x2_from_f32x2(f32x2_round(vector)));
}

vec2 operator+(vec2 lhs, vec2 rhs) {
    return vec2_from_f32x2(f32x2_add(lhs.vector, rhs.vector));
}
vec2& operator+=(vec2& lhs, vec2 rhs) {
    return lhs = vec2_from_f32x2(f32x2_add(lhs.vector, rhs.vector));
}
vec2 operator-(vec2 lhs, vec2 rhs) {
    return vec2_from_f32x2(f32x2_sub(lhs.vector, rhs.vector));
}
vec2 operator-(vec2 vec) {
    return vec2_from_f32x2(f32x2_negate(vec.vector));
}
vec2& operator-=(vec2& lhs, vec2 rhs) {
    return lhs = vec2_from_f32x2(f32x2_sub(lhs.vector, rhs.vector));
}
vec2 operator*(vec2 lhs, float rhs) {
    return vec2_from_f32x2(f32x2_scale(lhs.vector, rhs));
}
vec2 operator*(float lhs, vec2 rhs) {
    return vec2_from_f32x2(f32x2_scale(rhs.vector, lhs));
}
vec2 operator*(vec2 lhs, vec2 rhs) {
    return vec2_from_f32x2(f32x2_mul(lhs.vector, rhs.vector));
}
vec2& operator*=(vec2& lhs, float rhs) {
    return lhs = vec2_from_f32x2(f32x2_scale(lhs.vector, rhs));
}
vec2 operator/(vec2 lhs, float rhs) {
    return vec2_from_f32x2(f32x2_div(lhs.vector, f32x2_splat(rhs)));
}
vec2& operator/=(vec2& lhs, float rhs) {
    return lhs = vec2_from_f32x2(f32x2_div(lhs.vector, f32x2_splat(rhs)));
}

void print_value(Arena* out, vec2 v) {
    print_value(out, v.x);
    print_value(out, ',');
    print_value(out, v.y);
}

// -----------------------------------------------------------------------------
// ivec2

ivec2 ivec2::splat(int val) {
    return ivec2_from_i32x2(i32x2_splat(val));
}

int ivec2::manhattan() {
    return i32x2_add_across(i32x2_abs(vector));
}
ivec2 ivec2::abs() {
    return ivec2_from_i32x2(i32x2_abs(vector));
}
ivec2 ivec2::min(ivec2 rhs) {
    return ivec2_from_i32x2(i32x2_min(vector, rhs.vector));
}
ivec2 ivec2::max(ivec2 rhs) {
    return ivec2_from_i32x2(i32x2_max(vector, rhs.vector));
}
ivec2 ivec2::clamp(ivec2 min_inclusive, ivec2 max_exclusive) {
    i32x2 max = i32x2_sub(max_exclusive.vector, i32x2_splat(-1));
    return ivec2_from_i32x2(i32x2_max(min_inclusive.vector, i32x2_min(max, vector)));
}

vec2 ivec2::to_vec2() {
    return vec2_from_f32x2(f32x2_from_i32x2(vector));
}
uvec2 ivec2::to_uvec2_saturate() {
    ivec2 clamped = max(IVEC2_ZERO);
    return *(uvec2*)(&clamped);
}

ivec2 operator+(ivec2 lhs, ivec2 rhs) {
    return ivec2_from_i32x2(i32x2_add(lhs.vector, rhs.vector));
}
ivec2& operator+=(ivec2& lhs, ivec2 rhs) {
    return lhs = ivec2_from_i32x2(i32x2_add(lhs.vector, rhs.vector));
}
ivec2 operator-(ivec2 lhs, ivec2 rhs) {
    return ivec2_from_i32x2(i32x2_sub(lhs.vector, rhs.vector));
}
ivec2 operator-(ivec2 vec) {
    return ivec2_from_i32x2(i32x2_negate(vec.vector));
}
ivec2& operator-=(ivec2& lhs, ivec2 rhs) {
    return lhs = ivec2_from_i32x2(i32x2_sub(lhs.vector, rhs.vector));
}
ivec2 operator*(ivec2 lhs, int rhs) {
    return ivec2_from_i32x2(i32x2_scale(lhs.vector, rhs));
}
ivec2 operator*(int lhs, ivec2 rhs) {
    return ivec2_from_i32x2(i32x2_scale(rhs.vector, lhs));
}
ivec2 operator*(ivec2 lhs, ivec2 rhs) {
    return ivec2_from_i32x2(i32x2_mul(lhs.vector, rhs.vector));
}
ivec2& operator*=(ivec2& lhs, int rhs) {
    return lhs = ivec2_from_i32x2(i32x2_scale(lhs.vector, rhs));
}
bool operator==(ivec2 lhs, ivec2 rhs) {
    return u32x2_min_across((u32x2)i32x2_equal(lhs.vector, rhs.vector)) != 0;
}

void print_value(Arena* out, ivec2 v) {
    print_value(out, v.x);
    print_value(out, ',');
    print_value(out, v.y);
}

// -----------------------------------------------------------------------------
// uvec2

uvec2 uvec2::min(uvec2 rhs) {
    return uvec2_from_u32x2(u32x2_min(vector, rhs.vector));
}
uvec2 uvec2::max(uvec2 rhs) {
    return uvec2_from_u32x2(u32x2_max(vector, rhs.vector));
}

vec2 uvec2::to_vec2() {
    return vec2_from_f32x2(f32x2_from_u32x2(vector));
}
ivec2 uvec2::to_ivec2() {
    return ivec2((i32)x, (i32)y).max(IVEC2_ZERO);
}

uvec2 operator+(uvec2 lhs, uvec2 rhs) {
    return uvec2_from_u32x2(u32x2_add(lhs.vector, rhs.vector));
}
uvec2& operator+=(uvec2& lhs, uvec2 rhs) {
    return lhs = uvec2_from_u32x2(u32x2_add(lhs.vector, rhs.vector));
}
uvec2 operator-(uvec2 lhs, uvec2 rhs) {
    return uvec2_from_u32x2(u32x2_sub(lhs.vector, rhs.vector));
}
uvec2& operator-=(uvec2& lhs, uvec2 rhs) {
    return lhs = uvec2_from_u32x2(u32x2_sub(lhs.vector, rhs.vector));
}
uvec2 operator*(uvec2 lhs, u32 rhs) {
    return uvec2_from_u32x2(u32x2_scale(lhs.vector, rhs));
}
uvec2 operator*(u32 lhs, uvec2 rhs) {
    return uvec2_from_u32x2(u32x2_scale(rhs.vector, lhs));
}
uvec2 operator*(uvec2 lhs, uvec2 rhs) {
    return uvec2_from_u32x2(u32x2_mul(lhs.vector, rhs.vector));
}
uvec2& operator*=(uvec2& lhs, u32 rhs) {
    return lhs = uvec2_from_u32x2(u32x2_scale(lhs.vector, rhs));
}
bool operator==(uvec2 lhs, uvec2 rhs) {
    return u32x2_min_across((u32x2)u32x2_equal(lhs.vector, rhs.vector)) != 0;
}

void print_value(Arena* out, uvec2 v) {
    print_value(out, v.x);
    print_value(out, ',');
    print_value(out, v.y);
}

// -----------------------------------------------------------------------------
// vec3

vec3a vec3::to_vec3a() {
    return vec3a(x, y, z);
}

float vec3a::dot(vec3a rhs) {
    return f32x4_add_across(f32x4_mul(vector, rhs.vector));
}
vec3a vec3a::cross(vec3a rhs) {
    return vec3a(
        y * rhs.z - rhs.y * z,
        z * rhs.x - rhs.z * x,
        x * rhs.y - rhs.x * y
    );
}
vec3a vec3a::lerp(vec3a targ, float t) {
    return vec3a_from_f32x4(f32x4_scale_add(vector, f32x4_sub(targ.vector, vector), t));
}
vec3a vec3a::normalize() {
    float len = sqrtf(f32x4_add_across(f32x4_mul(vector, vector)));
    return vec3a_from_f32x4(f32x4_div(vector, f32x4_splat(len)));
}

vec3 vec3a::to_vec3() {
    return vec3(x, y, z);
}
vec4 vec3a::to_vec4(float w) {
    return vec4(x, y, z, w);
}

vec3a operator+(vec3a lhs, vec3a rhs) {
    return vec3a_from_f32x4(f32x4_add(lhs.vector, rhs.vector));
}
vec3a& operator+=(vec3a& lhs, vec3a rhs) {
    return lhs = vec3a_from_f32x4(f32x4_add(lhs.vector, rhs.vector));
}
vec3a operator-(vec3a lhs, vec3a rhs) {
    return vec3a_from_f32x4(f32x4_sub(lhs.vector, rhs.vector));
}
vec3a operator-(vec3a vec) {
    return vec3a_from_f32x4(f32x4_negate(vec.vector));
}
vec3a& operator-=(vec3a& lhs, vec3a rhs) {
    return lhs = vec3a_from_f32x4(f32x4_sub(lhs.vector, rhs.vector));
}
vec3a operator*(vec3a lhs, float rhs) {
    return vec3a_from_f32x4(f32x4_scale(lhs.vector, rhs));
}
vec3a operator*(float lhs, vec3a rhs) {
    return vec3a_from_f32x4(f32x4_scale(rhs.vector, lhs));
}
vec3a operator*(vec3a lhs, vec3a rhs) {
    return vec3a_from_f32x4(f32x4_mul(lhs.vector, rhs.vector));
}
vec3a& operator*=(vec3a& lhs, float rhs) {
    return lhs = vec3a_from_f32x4(f32x4_scale(lhs.vector, rhs));
}
vec3a operator/(vec3a lhs, float rhs) {
    return vec3a_from_f32x4(f32x4_div(lhs.vector, f32x4_splat(rhs)));
}
vec3a& operator/=(vec3a& lhs, float rhs) {
    return lhs = vec3a_from_f32x4(f32x4_div(lhs.vector, f32x4_splat(rhs)));
}

void print_value(Arena* out, vec3 v) {
    print_value(out, v.x);
    print_value(out, ',');
    print_value(out, v.y);
    print_value(out, ',');
    print_value(out, v.z);
}
void print_value(Arena* out, vec3a v) {
    print_value(out, v.x);
    print_value(out, ',');
    print_value(out, v.y);
    print_value(out, ',');
    print_value(out, v.z);
}

// -----------------------------------------------------------------------------
// ivec3

// -----------------------------------------------------------------------------
// vec4

float vec4::dot(vec4 rhs) {
    return f32x4_add_across(f32x4_mul(vector, rhs.vector));
}

vec4 operator+(vec4 lhs, vec4 rhs) {
    return vec4_from_f32x4(f32x4_add(lhs.vector, rhs.vector));
}
vec4& operator+=(vec4& lhs, vec4 rhs) {
    return lhs = vec4_from_f32x4(f32x4_add(lhs.vector, rhs.vector));
}
vec4 operator-(vec4 lhs, vec4 rhs) {
    return vec4_from_f32x4(f32x4_sub(lhs.vector, rhs.vector));
}
vec4 operator-(vec4 vec) {
    return vec4_from_f32x4(f32x4_negate(vec.vector));
}
vec4& operator-=(vec4& lhs, vec4 rhs) {
    return lhs = vec4_from_f32x4(f32x4_sub(lhs.vector, rhs.vector));
}
vec4 operator*(vec4 lhs, float rhs) {
    return vec4_from_f32x4(f32x4_scale(lhs.vector, rhs));
}
vec4 operator*(float lhs, vec4 rhs) {
    return vec4_from_f32x4(f32x4_scale(rhs.vector, lhs));
}
vec4 operator*(vec4 lhs, vec4 rhs) {
    return vec4_from_f32x4(f32x4_mul(lhs.vector, rhs.vector));
}
vec4& operator*=(vec4& lhs, float rhs) {
    return lhs = vec4_from_f32x4(f32x4_scale(lhs.vector, rhs));
}
vec4 operator/(vec4 lhs, float rhs) {
    return vec4_from_f32x4(f32x4_div(lhs.vector, f32x4_splat(rhs)));
}
vec4& operator/=(vec4& lhs, float rhs) {
    return lhs = vec4_from_f32x4(f32x4_div(lhs.vector, f32x4_splat(rhs)));
}

void print_value(Arena* out, vec4 v) {
    print_value(out, v.x);
    print_value(out, ',');
    print_value(out, v.y);
    print_value(out, ',');
    print_value(out, v.z);
    print_value(out, ',');
    print_value(out, v.w);
}

// -----------------------------------------------------------------------------
// ivec4

// -----------------------------------------------------------------------------
// mat2

mat2& mat2::mk_identity() {
    a = vec2(1, 0);
    b = vec2(0, 1);
    return *this;
}
mat2& mat2::mk_rotation(float radians) {
    float s, c;
    sincosf(radians, &s, &c);
    a = vec2(c, -s);
    b = vec2(s, c);
    return *this;
}
mat2& mat2::mk_rotation_pi_over_2() {
    a = vec2(0, -1);
    b = vec2(1, 0);
    return *this;
}
mat2& mat2::mk_rotation_pi() {
    a = vec2(-1, 0);
    b = vec2(0, -1);
    return *this;
}

vec2 operator*(mat2& lhs, vec2 rhs) {
    f32x4 v1 = f32x2_combine(rhs.vector, rhs.vector);
    f32x4 m1 = f32x2_combine(lhs.a.vector, lhs.b.vector);
    f32x4 prod = f32x4_mul(v1, m1);
    return vec2_from_f32x2(f32x2_add_pairs(f32x4_get_low(prod), f32x4_get_high(prod)));
}

void print_value(Arena* out, mat2 m) {
    print_value(out, m.a);
    print_value(out, ',');
    print_value(out, m.b);
}

// -----------------------------------------------------------------------------
// mat4

mat4& mat4::mk_identity() {
    a = vec4(1, 0, 0, 0);
    b = vec4(0, 1, 0, 0);
    c = vec4(0, 0, 1, 0);
    d = vec4(0, 0, 0, 1);
    return *this;
}

mat4& mat4::mk_ortho(float left, float right, float bottom, float top, float z_near, float z_far) {
    float rl, tb, fn;
    ZeroStruct(this);

    rl = 1.f / (right - left);
    tb = 1.f / (top - bottom);
    fn = -1.f / (z_far - z_near);

    a.x = 2.f * rl;
    b.y = 2.f * tb;
    c.z = 2.f * fn;
    d.x = -(right + left) * rl;
    d.y = -(top + bottom) * tb;
    d.z = (z_far + z_near) * fn;
    d.w = 1.f;

    return *this;
}

mat4& mat4::mk_perspective(float fov_y, float aspect, float z_near, float z_far) {
    float tan_half_fov_y = tanf(fov_y / 2.f);

    ZeroStruct(this);
    a.x = 1.f / (aspect * tan_half_fov_y);
    // if (vulkan , ie NDC -1 top, +1 bottom) {
    //    b.y = -1.f / tan_half_fov_y;
    // } else {
    b.y = 1.f / tan_half_fov_y;
    // }
    c.z = z_far / (z_near - z_far);
    c.w = -1.f;
    d.z = -(z_far * z_near) / (z_far - z_near);
    return *this;
}

mat4& mat4::mk_rotation_angle_axis(float angle, vec3a normalized_axis) {
    float co = cosf(angle);
    vec3a v = normalized_axis * (1.f - co);
    vec3a vs = normalized_axis * sinf(angle);
    a = (normalized_axis * v.x).to_vec4(0.f);
    b = (normalized_axis * v.y).to_vec4(0.f);
    c = (normalized_axis * v.z).to_vec4(0.f);

    a.x += co;
    b.x -= vs.z;
    c.x += vs.y;
    a.y += vs.z;
    b.y += co;
    c.y -= vs.x;
    a.z -= vs.y;
    b.z += vs.x;
    c.z += co;

    a.w = b.w = c.w = d.x = d.y = d.z = 0.f;
    d.w = 1.f;

    return *this;
}

mat4& mat4::mk_look_at(vec3a eye, vec3a target, vec3a up) {
    vec3a f = (target - eye).normalize();
    vec3a s = f.cross(up).normalize();
    vec3a u = s.cross(f);

    a.x = s.x;
    b.x = s.y;
    c.x = s.z;
    d.x = -s.dot(eye);

    a.y = u.x;
    b.y = u.y;
    c.y = u.z;
    d.y = -u.dot(eye);

    a.z = -f.x;
    b.z = -f.y;
    c.z = -f.z;
    d.z = f.dot(eye);

    a.w = 0.f;
    b.w = 0.f;
    c.w = 0.f;
    d.w = 1.f;

    return *this;
}

mat4& mat4::mk_view(vec3a camera_pos, vec3a camera_fwd_normalized, vec3a camera_up_normalized) {
    vec3a f = camera_fwd_normalized;
    vec3a s = f.cross(camera_up_normalized).normalize();
    vec3a u = s.cross(f);

    a.x = s.x;
    b.x = s.y;
    c.x = s.z;
    d.x = -s.dot(camera_pos);

    a.y = u.x;
    b.y = u.y;
    c.y = u.z;
    d.y = -u.dot(camera_pos);

    a.z = -f.x;
    b.z = -f.y;
    c.z = -f.z;
    d.z = f.dot(camera_pos);

    a.w = 0.f;
    b.w = 0.f;
    c.w = 0.f;
    d.w = 1.f;

    return *this;
}

mat4& mat4::apply_scale(vec3a scale) {
    a *= scale.x;
    b *= scale.y;
    c *= scale.z;
    return *this;
}

mat4& mat4::apply_translation(vec3a translate) {
    d.vector = f32x4_add(d.vector, translate.vector);
    return *this;
}

mat4 operator*(mat4 lhs, mat4 rhs) {
    mat4 ret = lhs;
    return ret *= rhs;
}

vec4 mat4::mul_vec4(vec4 rhs) {
    return vec4(
        a.dot(rhs),
        b.dot(rhs),
        c.dot(rhs),
        d.dot(rhs)
    );
}

mat4& operator*=(mat4& lhs, mat4 rhs) {
    f32x4 r0 = rhs.a.vector;
    f32x4 r1 = rhs.b.vector;
    f32x4 r2 = rhs.c.vector;
    f32x4 r3 = rhs.d.vector;

    f32x4 l = lhs.a.vector;
    f32x4 v0 = f32x4_scale(l, f32x4_get_lane(r0, 0));
    f32x4 v1 = f32x4_scale(l, f32x4_get_lane(r1, 0));
    f32x4 v2 = f32x4_scale(l, f32x4_get_lane(r2, 0));
    f32x4 v3 = f32x4_scale(l, f32x4_get_lane(r3, 0));

    l = lhs.b.vector;
    v0 = f32x4_scale_add(v0, l, f32x4_get_lane(r0, 1));
    v1 = f32x4_scale_add(v1, l, f32x4_get_lane(r1, 1));
    v2 = f32x4_scale_add(v2, l, f32x4_get_lane(r2, 1));
    v3 = f32x4_scale_add(v3, l, f32x4_get_lane(r3, 1));

    l = lhs.c.vector;
    v0 = f32x4_scale_add(v0, l, f32x4_get_lane(r0, 2));
    v1 = f32x4_scale_add(v1, l, f32x4_get_lane(r1, 2));
    v2 = f32x4_scale_add(v2, l, f32x4_get_lane(r2, 2));
    v3 = f32x4_scale_add(v3, l, f32x4_get_lane(r3, 2));

    l = lhs.d.vector;
    v0 = f32x4_scale_add(v0, l, f32x4_get_lane(r0, 3));
    v1 = f32x4_scale_add(v1, l, f32x4_get_lane(r1, 3));
    v2 = f32x4_scale_add(v2, l, f32x4_get_lane(r2, 3));
    v3 = f32x4_scale_add(v3, l, f32x4_get_lane(r3, 3));

    lhs.a.vector = v0;
    lhs.b.vector = v1;
    lhs.c.vector = v2;
    lhs.d.vector = v3;

    return lhs;
}

void print_value(Arena* out, mat4 m) {
    print_value(out, m.a);
    print_value(out, ',');
    print_value(out, m.b);
    print_value(out, ',');
    print_value(out, m.c);
    print_value(out, ',');
    print_value(out, m.d);
}

// -----------------------------------------------------------------------------

float f32_lerp(float a, float b, float t) {
    return a + t * (b - a);
}

float f32_fract(float a) {
    return a - floorf(a);
}

i64 i64_mod(i64 a, i64 b) {
    i64 ret = a % b;
    if (ret < 0) ret += b;
    return ret;
}

float radians_sub(float lhs, float rhs) {
    float diff = fmodf(lhs - rhs, 2.f * M_PI);

    if (diff > M_PI) {
        diff -= 2.f * M_PI;
    } else if (diff < -M_PI) {
        diff += 2.f * M_PI;
    }

    return diff;
}

float radians_lerp(float a, float b, float t) {
    return a + t * radians_sub(b, a);
}

float radians_mod_zero_to_2pi(float rads) {
    rads = fmodf(rads, 2.f * M_PI);
    if (rads < 0) rads += 2.f * M_PI;
    return rads;
}

// given some sinusoidal function f with period 2pi, but with unknown phase/amplitude/dc, return the
// input to that function in the range 0-2pi which corresponds to the minimum output value.
float radians_minimize_unknown_sine(float (*f)(float)) {
    float f0 = f(0);
    float f1 = f(M_PI_2);
    float f2 = f(M_PI);

    float A = 0.5f * f0 - 0.5f * f2;
    float B = -0.5f * f0 + f1 - 0.5f * f2;

    float min_angle = 3.f * M_PI_2 - atan2f(A, B);
    if (min_angle > 2.f * M_PI) min_angle -= 2.f * M_PI;

    return min_angle;
}

// -----------------------------------------------------------------------------

float ease_out_back(float x) {
    float c1 = 1.70158f;
    float c3 = c1 + 1.f;
    float z = x - 1.f;
    return 1.f + c3 * z * z * z + c1 * z * z;
}

float ease_out_quad(float x) {
    float u = 1.f - x;
    return 1.f - u * u;
}

float ease_in_quad(float x) {
    return x * x;
}

float ease_out_cubic(float x) {
    float u = 1.f - x;
    return 1.f - u * u * u;
}

float ease_in_cubic(float x) {
    return x * x * x;
}

float ease_in_out_quad(float x) {
    float a = -2.f * x + 2.f;
    return x < .5f ? 2.f * x * x : 1.f - a * a * .5f;
}

// -----------------------------------------------------------------------------
namespace geo {

BresenhamIter BresenhamIter::make(ivec2 p0, ivec2 p1) {
    BresenhamIter ret = {};
    ret.item = p0;
    ret.p1 = p1;
    ret.d = ivec2(abs(p1.x - p0.x), -abs(p1.y - p0.y));
    ret.s = ivec2(p0.x < p1.x ? 1 : -1, p0.y < p1.y ? 1 : -1);
    ret.err = ret.d.x + ret.d.y;
    return ret;
}

void BresenhamIter::next() {
    if (item.x == p1.x && item.y == p1.y) {
        done = true;
        return;
    }
    int e2 = 2 * err;
    if (e2 >= d.y) {
        err += d.y;
        item.x += s.x;
    }
    if (e2 <= d.x) {
        err += d.x;
        item.y += s.y;
    }
}

LineSegPointResult closest_point_on_line_seg(vec2 pt, vec2 line0, vec2 line1) {
    LineSegPointResult result = {};

    vec2 to_pt = pt - line0;
    vec2 line_vec = line1 - line0;

    float dot = to_pt.dot(line_vec);
    float len_sqr = line_vec.dot(line_vec);
    float t = -1.f;

    if (len_sqr != 0.f) t = dot / len_sqr;

    if (t <= 0.f) {
        result.on_line = false;
        result.pt = line0;
    } else if (t >= 1.f) {
        result.on_line = false;
        result.pt = line1;
    } else {
        result.on_line = true;
        result.pt = line0.add_scaled(t, line_vec);
    }

    return result;
}

bool rect_overlaps_circle(vec2 min, vec2 max, vec2 circle_center, float circle_radius) {
    vec2 delta = circle_center - circle_center.clamp(min, max);
    return delta.length_sqr() < circle_radius * circle_radius;
}

bool rect_overlaps_rect(vec2 min0, vec2 max0, vec2 min1, vec2 max1) {
    return max1.x > min0.x && min1.x < max0.x && max1.y > min0.y && min1.y < max0.y;
}

LineSegIntersectResult line_hit_oriented_line(vec2 a0, vec2 a1, vec2 b0, vec2 b1) {
    LineSegIntersectResult result = {};

    vec2 r = a1 - a0;
    vec2 s = b1 - b0;
    vec2 ba = b0 - a0;
    float rxs = r.cross(s);

    // only include hits of line A against the left side of line B
    if (rxs <= 0.f) return result;

    float t = ba.cross(s) / rxs;
    if (t < 0.f || t > 1.f) return result;

    float u = ba.cross(r) / rxs;
    if (u < 0.f || u > 1.f) return result;

    result.hit = true;
    result.point = a0.add_scaled(t, r);
    result.normal = vec2(-s.y, s.x);
    result.t = t;
    return result;
}

LineSegIntersectResult line_hit_circle(vec2 p0, vec2 p1, vec2 center, float radius) {
    LineSegIntersectResult result = {};

    vec2 d = p1 - p0;
    vec2 f = p0 - center;

    float a = d.length_sqr();
    float b = 2.f * f.dot(d);
    float c = f.length_sqr() - radius * radius;
    float disc = b * b - 4.f * a * c;

    if (disc < 0.f) return result;

    disc = sqrtf(disc);
    float t0 = (-b - disc) / (2.f * a);
    float t1 = (-b + disc) / (2.f * a);

    if (t1 < 0.f || t0 > 1.f) return result;

    float t = clamp01(t0);
    vec2 point = p0.add_scaled(t, d);

    result.hit = true;
    result.point = point;
    result.normal = (point - center).normalize();
    result.t = t;
    return result;
}

LineSegIntersectResult line_hit_rect(vec2 p0, vec2 p1, vec2 rect_min, vec2 rect_max) {
    LineSegIntersectResult result = {};

    if (rect_contains_point(rect_min, rect_max, p0)) {
        result.hit = true;
        result.normal = (p0 - p1).normalize();
        result.point = p0;
        return result;
    }

    result = line_hit_oriented_line(p0, p1, rect_min, vec2(rect_min.x, rect_max.y));
    if (result.hit) return result;
    result = line_hit_oriented_line(p0, p1, vec2(rect_min.x, rect_max.y), rect_max);
    if (result.hit) return result;
    result = line_hit_oriented_line(p0, p1, rect_max, vec2(rect_max.x, rect_min.y));
    if (result.hit) return result;
    result = line_hit_oriented_line(p0, p1, vec2(rect_max.x, rect_min.y), rect_min);
    if (result.hit) return result;

    return LineSegIntersectResult{};
}

bool rect_contains_point(vec2 rect_min, vec2 rect_max, vec2 pt) {
    return pt.x >= rect_min.x &&
        pt.y >= rect_min.y &&
        pt.x <= rect_max.x &&
        pt.y <= rect_max.y;
}

}  // namespace geo
// -----------------------------------------------------------------------------
namespace random {

int range(int min_inclusive, int max_exclusive) {
    int delta = max_exclusive - min_inclusive;
    return min_inclusive + (int)(value() * delta);
}

float value() {
    // linear congruential prng, seeded by cpu ticks
    u32 seed = (u32)timing_get_ticks() ^ 0x7AC0BE11;
    seed = wrapped_add(wrapped_mul(seed, 1664525), 1013904223);
    return (float)((double)seed / 4294967296.0);
}

}  // namespace random
// -----------------------------------------------------------------------------

forall(T) void flip_grid_subset_x(T* elems, uvec2 grid_size, uvec2 subset_pos, uvec2 subset_size) {
    Assert(subset_pos.x + subset_size.x <= grid_size.x);
    Assert(subset_pos.y + subset_size.y <= grid_size.y);

    for (int iy = 0, my = subset_size.y; iy < my; ++iy) {
        int off = (iy + subset_pos.y) * grid_size.x + subset_pos.x;
        for (int ix = 0, mx = subset_size.x / 2; ix < mx; ++ix) {
            int jx = subset_size.x - 1 - ix;
            Swap(
                elems[ix + off],
                elems[jx + off]
            );
        }
    }
}

forall(T) void flip_grid_subset_y(T* elems, uvec2 grid_size, uvec2 subset_pos, uvec2 subset_size) {
    Assert(subset_pos.x + subset_size.x <= grid_size.x);
    Assert(subset_pos.y + subset_size.y <= grid_size.y);

    for (int iy = 0, my = subset_size.y / 2; iy < my; ++iy) {
        int jy = subset_size.y - 1 - iy;
        int off0 = (iy + subset_pos.y) * grid_size.x + subset_pos.x;
        int off1 = (jy + subset_pos.y) * grid_size.x + subset_pos.x;
        for (int ix = 0, mx = subset_size.x; ix < mx; ++ix) {
            Swap(
                elems[off0 + ix],
                elems[off1 + ix]
            );
        }
    }
}

// -----------------------------------------------------------------------------
}  // namespace a
