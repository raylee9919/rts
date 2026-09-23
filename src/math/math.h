// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RTS_MATH_H
#define RTS_MATH_H

#include "basic/core.h"


//
// sin/cos usually have high precision, but they still differ across CRT implementations.
// So it is better for me to roll my own to guarantee consistency across platforms.
// sqrt nowadays is a single CPU instruction, there's nothing else to implement.
// You just use correct builtin/intrinsic from compiler.
//


#define pi32                3.141592f
#define epsilon_f32         1.19209e-07f


// @Robustness
#define _FAR_Z  ( 1.f)
#define _NEAR_Z (-1.f)


union vec2 {
    struct { f32 x, y; };
    f32 e[2];

    vec2() = default;
    vec2(f32 x_, f32 y_) : x(x_), y(y_) {};
    vec2(f32 f)          : x( f), y( f) {};
};

struct vec3 {
    union {
        struct { f32 x, y, z; };
        f32 e[3];
    };

    vec3() = default;
    vec3(f32 x_, f32 y_, f32 z_)  : x(x_), y(y_), z(z_) {};
    vec3(f32 f)                   : x( f), y( f), z( f) {};
};

union vec4 {
    struct {
        union {
            struct { f32 r, g, b; };
            vec3 rgb;
        };
        f32 a;
    };
    struct {
        union {
            struct {
                union {
                    vec2 xy;
                    struct { f32 x, y; };
                };
                f32 z;
            };
            vec3 xyz;
        };
        f32 w;
    };
    f32 e[4];
#if SSE_ENABLED
    __m128 sse;
#endif

    vec4() = default;
    vec4(f32 f1, f32 f2, f32 f3, f32 f4) : x(f1), y(f2), z(f3), w(f4) {};
    vec4(f32 f)                          : x( f), y( f), z( f), w( f) {};
};

union m4x4 {
    f32 e[4][4];
    struct {
        f32 _11, _12, _13, _14; 
        f32 _21, _22, _23, _24; 
        f32 _31, _32, _33, _34; 
        f32 _41, _42, _43, _44; 
    };
    vec4 rows[4];
};

union m3x4 {
    struct {
        f32 _11, _12, _13, _14; 
        f32 _21, _22, _23, _24; 
        f32 _31, _32, _33, _34; 
    };
    vec4 rows[3];
};

union Quaternion {
    struct { f32 w, x, y, z; };
    __m128 sse;

    Quaternion();
    Quaternion(f32 w_, f32 x_, f32 y_, f32 z_);
};

struct Xform {
    vec3         translation;
    Quaternion rotation;
    vec3         scale;

    Xform();
};

struct Rect2 {
    vec2 min, max;
};

struct AABB2 {
    vec2 min;
    vec2 max;
};

struct Ray3 {
    vec3 origin;
    vec3 direction;
};

// Sloppy
f32        safe_ratio(f32 l, f32 r);

// Trigonometry
f32        m_sin(f32 f);
f32        m_cos(f32 f);

// Absolute
f32        m_abs(f32 f);
f64        m_abs(f64 d);

// Square roots
f32        m_sqrt(f32 f);
f32        m_rsqrt(f32 f);

// Mapping functions
f32        map(f32 x, f32 min, f32 max);
f32        map_unorm(f32 x, f32 min, f32 max);
f32        map_snorm(f32 x, f32 min, f32 max);

// Easing functions
f32        lerp(f32 a, f32 b, f32 t);
f32        smoothstep(f32 min, f32 max, f32 x);
f32        hermite(f32 min, f32 max, f32 x);

// Vector2
vec2         operator  - (vec2& in);
vec2         operator  * (f32 f,  vec2 v);
vec2         operator  * (vec2  v, f32 f);
vec2         operator  + (vec2  l,  vec2 r);
vec2         operator  - (vec2  l,  vec2 r);
vec2&        operator += (vec2& l,  vec2 r);
vec2&        operator -= (vec2& l,  vec2 r);
vec2&        operator *= (vec2& v, f32 f);
vec2         operator  * (vec2  l,  vec2 r); // Hadamard product. I'm just following the shader convention.


//
// @Todo: Cleanup below.
//

#if SSE_ENABLED
f32            dot(__m128 a, __m128 b);
#endif
f32            dot(vec2 a, vec2 b);
f32            dot(vec3 a, vec3 b);
f32            dot(vec4 a, vec4 b);
f32            dot(Quaternion a, Quaternion b);

vec3           cross(vec3 a, vec3 b);

vec3           hadamard(vec3 a, vec3 b);
vec4           hadamard(vec4 a, vec4 b);

f64            fmod_cycling(f64 x, f64 y);
f32            fmod_cycling(f32 x, f32 y);

f32            sqlen(vec2 v);
f32            sqlen(vec3 v);

f32            invsqlen(vec2 v);
f32            invsqlen(vec3 v);
f32            invsqlen(vec4 v);

f32            length(vec2 A);
vec2           normalize(vec2 a);
vec2           lerp(vec2 a, vec2 b, f32 t);
vec3           operator - (const vec3 &in);
vec3           operator * (f32 A, vec3 B);
vec3           operator * (vec3 B, f32 A);
vec3           operator / (vec3 a, f32 b);
vec3&          operator /= (vec3& a, f32 b);
vec3           operator + (vec3 A, vec3 B);
vec3           operator - (vec3 A, vec3 B);
vec3&          operator += (vec3& a, vec3 b);
vec3&          operator -= (vec3& a, vec3 b);
vec3&          operator *= (vec3& a, f32 b);
f32            length(vec3 A);
b32            is_zero(vec3 v);
b32            is_inf(vec3 v);
vec3           normalize(vec3 a);
vec3           lerp(vec3 a, vec3 b, f32 t);
f32            distance(vec3 a, vec3 b);
f32            distance(vec2 a, vec2 b);
vec4           V4(f32 x);
vec4           V4(f32 r, f32 g, f32 b, f32 a);
vec4           V4(vec2 rg, f32 b, f32 a);
vec4           V4(vec3 rgb, f32 a);
vec4           operator * (vec4 v, f32 f);
vec4           operator * (f32 a, vec4 v);
vec4           lerp(vec4 a, vec4 b, f32 t);
Quaternion     operator + (Quaternion a, Quaternion b);
Quaternion     operator - (Quaternion l, Quaternion r);
Quaternion     operator * (Quaternion a, Quaternion b);
Quaternion     operator * (Quaternion a, f32 b);
Quaternion     operator * (f32 b, Quaternion a);
Quaternion     operator - (Quaternion in);
Quaternion     normalize(Quaternion q);
Quaternion     nlerp(Quaternion a, Quaternion b, f32 t);
Quaternion     slerp(Quaternion q1, Quaternion q2, f32 t);
m4x4           operator * (m4x4 a, m4x4 b);
m4x4&          operator *= (m4x4& m, f32 f);
vec4           operator * (m4x4 m, vec4 p);
m4x4           operator * (m4x4 m, f32 f);
m4x4           operator * (f32 f, m4x4 m);
m4x4           identity();
m4x4           x_rotation(f32 a);
m4x4           y_rotation(f32 a);
m4x4           z_rotation(f32 a);
m4x4           transpose(m4x4 m);
m4x4           inverse(m4x4 m);
m4x4           rows(vec3 x, vec3 y, vec3 z);
m4x4           columns(vec3 x, vec3 y, vec3 z);

m4x4           m4x4_translate(f32 x, f32 y, f32 z);
m4x4           m4x4_translate(vec3 t);
m4x4           m4x4_translate(m4x4 m, vec3 t);

m4x4           to_m4x4(Quaternion q);
Quaternion     euler_to_quaternion(f32 roll, f32 pitch, f32 yaw);
m4x4           scale(m4x4 transform, vec3 factor);
m4x4           scale(m4x4 transform, f32 factor);
m4x4           scale(f32 s);
m4x4           m4x4_scale(f32 x, f32 y, f32 z);
Rect2          rect2_min_max(vec2 min, vec2 max);
Rect2          rect2_cen_half_dim(vec2 cen, vec2 h_dim);
Rect2          rect2_min_dim(vec2 min, vec2 dim);
Rect2          rect2_inv_inf();
Rect2          offset(Rect2 rect, vec2 offset);
Rect2          add_radius_to(Rect2 rect, vec2 radius);
m4x4           to_m4x4(Xform xform);
m4x4           to_m4x4(vec3 translation, Quaternion rotation, vec3 scale);
Quaternion     build_quaternion(vec3 axis, f32 radian);
Quaternion     rotate(Quaternion q0, vec3 axis, f32 radian);
vec3           project(vec3 p, m4x4 view_proj);



m4x4           ortho(f32 min_x, f32 max_x, f32 min_y, f32 max_y, f32 min_z, f32 max_z);
f32            radian_from_degree(f32 d);

f32            normalize01(vec2 range, f32 val);
b32            intersects(AABB2 box, vec2 point);
b32            intersects(AABB2 a, AABB2 b);
AABB2          intersection(AABB2 a, AABB2 b);
AABB2          aabb2_infinite(void);

vec2           to_ndc(vec2 p, f32 w, f32 h);
vec3           unproject(vec3 position, m4x4 viewproj);
Ray3           ray_from_screen_position(vec2 position, f32 screen_width, f32 screen_height, m4x4 viewproj);


//
// Graphics
//
// I like to view the process of building look-at matrix as determining the
// local axes of a camera placed in the world. The axes are the camera's own
// language, into which the objects' positions, expressed in the world's
// language are translated.
//
// The problem is that axes can be arbitrary. Just imagine three orthogonal
// axes rotating rotating wildly around the camera. Thus, we must impose some
// constraints.
//
// As most, if not all, graphics APIs' viewport is X-right and Y-up, these will
// be the premises in the API. Of course, I provide additional functionality to
// customize them, but IMO, the extra verbosity isn't worth it.
//
// Now, all we have to determine is Z. Does it point "foward" or "backward"?
// In other words, is it left-handed or right-handed? This is only variant exposed by the API.
//
m4x4 look_to_lh(vec3 from, vec3 to, vec3 up);
m4x4 look_to_rh(vec3 from, vec3 to, vec3 up);
m4x4 look_at_lh(vec3 from, vec3 at, vec3 up);
m4x4 look_at_rh(vec3 from, vec3 at, vec3 up);

// For perspective projection as well, we impose the following premises: [0,1]
// for NDC depth, X-right and Y-up. The projection maps 'near_z' to 0 and
// 'far_z' to 1. 
// 
// LH and RH variants specify the handedness of the input space. The 'near_z' and 
// 'far_z' parameters are distances, and are therefore unsigned.
//
m4x4 persp_fov_lh(f32 fov, f32 aspect_ratio, f32 near_z, f32 far_z);
m4x4 persp_fov_rh(f32 fov, f32 aspect_ratio, f32 near_z, f32 far_z);

u32  pack_rgba(vec4 rgba);
vec4 unpack_rgba(u32 rgba);


f32           m_tan(f32 f);
f32           triarea2(vec2 a, vec2 b, vec2 c);
vec4          operator + (vec4 a, vec4 b);
m4x4&         operator += (m4x4& l, m4x4 r);
bool          ray_plane_intersect(Ray3 ray, vec3 plane_normal, f32 plane_height, vec3* out);
Xform         to_xform(m4x4 m);

#endif // RTS_MATH_H
