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


union v2 {
    struct { f32 x, y; };
    f32 e[2];

    v2() = default;
    v2(f32 x_, f32 y_);
    v2(f32 f);
};

union v2s { 
    struct { s32 x, y; };
    s32 e[2];
};

union v2u {
    struct {u32 x, y;};
    struct {u32 w, h;};
    u32 e[2];

};

struct v3 {
    union {
        struct { f32 x, y, z; };
        f32 e[3];
    };

    v3() = default;
    v3(f32 f);
    v3(f32 x_, f32 y_, f32 z_);
    v3(v2 xy, f32 z_);
};

union v4 {
    struct {
        union {
            struct { f32 r, g, b; };
            v3 rgb;
        };
        f32 a;
    };
    struct {
        union {
            struct {
                union {
                    v2 xy;
                    struct { f32 x, y; };
                };
                f32 z;
            };
            v3 xyz;
        };
        f32 w;
    };
    f32 e[4];
#if SSE_ENABLED
    __m128 sse;
#endif

    v4() = default;
    v4(f32 f1, f32 f2, f32 f3, f32 f4);
    v4(f32 f);
};

union m4x4 {
    f32 e[4][4];
    struct {
        f32 _11, _12, _13, _14; 
        f32 _21, _22, _23, _24; 
        f32 _31, _32, _33, _34; 
        f32 _41, _42, _43, _44; 
    };
    v4 rows[4];
};

union m3x4 {
    struct {
        f32 _11, _12, _13, _14; 
        f32 _21, _22, _23, _24; 
        f32 _31, _32, _33, _34; 
    };
    v4 rows[3];
};

union Quaternion {
    struct { f32 w, x, y, z; };
    __m128 sse;

    Quaternion();
    Quaternion(f32 w_, f32 x_, f32 y_, f32 z_);
};

struct Xform {
    v3         translation;
    Quaternion rotation;
    v3         scale;

    Xform();
};

struct Rect2 {
    v2 min, max;
};

struct AABB2 {
    v2 min;
    v2 max;
};

struct Ray3 {
    v3 origin;
    v3 direction;
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
f32        lerp(f32 a, f32 t, f32 b);
f32        smoothstep(f32 min, f32 max, f32 x);
f32        hermite(f32 min, f32 max, f32 x);

// Vector2
v2         operator  - (v2& in);
v2         operator  * (f32 f,  v2 v);
v2         operator  * (v2  v, f32 f);
v2         operator  + (v2  l,  v2 r);
v2         operator  - (v2  l,  v2 r);
v2&        operator += (v2& l,  v2 r);
v2&        operator -= (v2& l,  v2 r);
v2&        operator *= (v2& v, f32 f);
v2         operator  * (v2  l,  v2 r); // Hadamard product. I'm just following the shader convention.


//
// @Todo: Cleanup below.
//

#if SSE_ENABLED
f32            dot(__m128 a, __m128 b);
#endif
f32            dot(v2 a, v2 b);
f32            dot(v3 a, v3 b);
f32            dot(v4 a, v4 b);
f32            dot(Quaternion a, Quaternion b);

v3             cross(v3 a, v3 b);

v2             hadamard(v2 a, v2 b);
v3             hadamard(v3 a, v3 b);
v4             hadamard(v4 a, v4 b);

f64            fmod_cycling(f64 x, f64 y);
f32            fmod_cycling(f32 x, f32 y);

f32            sqlen(v2 v);
f32            sqlen(v3 v);

f32            invsqlen(v2 v);
f32            invsqlen(v3 v);
f32            invsqlen(v4 v);

f32            length(v2 A);
v2             normalize(v2 a);
v2             lerp(v2 a, f32 t, v2 b);
v3             operator - (const v3 &in);
v3             operator * (f32 A, v3 B);
v3             operator * (v3 B, f32 A);
v3             operator / (v3 a, f32 b);
v3&            operator /= (v3& a, f32 b);
v3             operator + (v3 A, v3 B);
v3             operator - (v3 A, v3 B);
v3&            operator += (v3& a, v3 b);
v3&            operator -= (v3& a, v3 b);
v3&            operator *= (v3& a, f32 b);
f32            length(v3 A);
b32            is_zero(v3 v);
b32            is_inf(v3 v);
v3             normalize(v3 a);
v3             lerp(v3 a, f32 t, v3 b);
f32            distance(v3 a, v3 b);
f32            distance(v2 a, v2 b);
v4             V4(f32 x);
v4             V4(f32 r, f32 g, f32 b, f32 a);
v4             V4(v2 rg, f32 b, f32 a);
v4             V4(v3 rgb, f32 a);
v4             operator * (v4 v, f32 f);
v4             operator * (f32 a, v4 v);
v4             lerp(v4 a, f32 t, v4 b);
Quaternion     operator + (Quaternion a, Quaternion b);
Quaternion     operator - (Quaternion l, Quaternion r);
Quaternion     operator * (Quaternion a, Quaternion b);
Quaternion     operator * (Quaternion a, f32 b);
Quaternion     operator * (f32 b, Quaternion a);
Quaternion     operator - (Quaternion in);
Quaternion     normalize(Quaternion q);
Quaternion     nlerp(Quaternion a, f32 t, Quaternion b);
Quaternion     slerp(Quaternion q1, f32 t, Quaternion q2);
m4x4           operator * (m4x4 a, m4x4 b);
m4x4&          operator *= (m4x4& m, f32 f);
v4             operator * (m4x4 m, v4 p);
m4x4           operator * (m4x4 m, f32 f);
m4x4           operator * (f32 f, m4x4 m);
m4x4           identity();
m4x4           x_rotation(f32 a);
m4x4           y_rotation(f32 a);
m4x4           z_rotation(f32 a);
m4x4           transpose(m4x4 m);
m4x4           inverse(m4x4 m);
m4x4           rows(v3 x, v3 y, v3 z);
m4x4           columns(v3 x, v3 y, v3 z);

m4x4           m4x4_translate(f32 x, f32 y, f32 z);
m4x4           m4x4_translate(v3 t);
m4x4           m4x4_translate(m4x4 m, v3 t);

m4x4           to_m4x4(Quaternion q);
Quaternion     euler_to_quaternion(f32 roll, f32 pitch, f32 yaw);
m4x4           scale(m4x4 transform, v3 factor);
m4x4           scale(m4x4 transform, f32 factor);
m4x4           scale(f32 s);
m4x4           m4x4_scale(f32 x, f32 y, f32 z);
Rect2          rect2_min_max(v2 min, v2 max);
Rect2          rect2_cen_half_dim(v2 cen, v2 h_dim);
Rect2          rect2_min_dim(v2 min, v2 dim);
Rect2          rect2_inv_inf();
Rect2          offset(Rect2 rect, v2 offset);
Rect2          add_radius_to(Rect2 rect, v2 radius);
m4x4           to_m4x4(Xform xform);
m4x4           to_m4x4(v3 translation, Quaternion rotation, v3 scale);
Quaternion     build_quaternion(v3 axis, f32 radian);
Quaternion     rotate(Quaternion q0, v3 axis, f32 radian);
v3             project(v3 p, m4x4 view_proj);



m4x4           ortho(f32 min_x, f32 max_x, f32 min_y, f32 max_y, f32 min_z, f32 max_z);
f32            radian_from_degree(f32 d);

f32            normalize01(v2 range, f32 val);
b32            intersects(AABB2 box, v2 point);
b32            intersects(AABB2 a, AABB2 b);
AABB2          intersection(AABB2 a, AABB2 b);
AABB2          aabb2_infinite(void);

v2             to_ndc(v2 p, f32 w, f32 h);
v3             unproject(v3 position, m4x4 viewproj);
Ray3           ray_from_screen_position(v2 position, f32 screen_width, f32 screen_height, m4x4 viewproj);


// Graphics

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
m4x4 look_to_lh(v3 from, v3 to, v3 up);
m4x4 look_to_rh(v3 from, v3 to, v3 up);
m4x4 look_at_lh(v3 from, v3 at, v3 up);
m4x4 look_at_rh(v3 from, v3 at, v3 up);

// For perspective projection as well, we impose the following premises: [0,1]
// for NDC depth, X-right and Y-up. The projection maps 'near_z' to 0 and
// 'far_z' to 1. 
// 
// LH and RH variants specify the handedness of the input space. The 'near_z' and 
// 'far_z' parameters are distances, and are therefore unsigned.
//
m4x4 persp_fov_lh(f32 fov, f32 aspect_ratio, f32 near_z, f32 far_z);
m4x4 persp_fov_rh(f32 fov, f32 aspect_ratio, f32 near_z, f32 far_z);

u32  pack_rgba(v4 rgba);
v4   unpack_rgba(u32 rgba);


f32           m_tan(f32 f);
f32           triarea2(v2 a, v2 b, v2 c);
v2            V2(v2u v);
v4            operator + (v4 a, v4 b);
m4x4&         operator += (m4x4& l, m4x4 r);
bool          ray_plane_intersect(Ray3 ray, v3 plane_normal, f32 plane_height, v3* out);
Xform         to_xform(m4x4 m);

#endif // RTS_MATH_H
