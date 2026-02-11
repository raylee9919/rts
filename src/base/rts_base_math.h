// Copyright Seong Woo Lee. All Rights Reserved.

#pragma once

#define pi32                3.141592f
#define epsilon_f32         1.19209e-07f

// @Robustness
#define FAR_Z  ( 1.f)
#define NEAR_Z (-1.f)


// # Note: Vectors
//
union v2 {
    struct { f32 x, y; };
    f32 e[2];

    v2() = default;
    v2(f32 x_, f32 y_);
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

struct Quaternion {
    f32 w, x, y, z;

    Quaternion() = default;
    Quaternion(f32 w_, f32 x_, f32 y_, f32 z_);
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

#define absolute(x) ((x) > 0 ? (x) : -(x))

internal f32 map(f32 x, f32 min, f32 max);
internal f32 map01(f32 x, f32 min, f32 max);
internal f32 map01_binormal(f32 x, f32 min, f32 max);
internal f32 binormal_to_normal(f32 x);

internal f32 lerp(f32 a, f32 t, f32 b);

internal f32 smoothstep(f32 x, f32 min, f32 max);
internal f32 safe_ratio(f32 a, f32 b);
internal v2 V2(f32 x, f32 y);
internal v2 V2(f32 x);
internal v2 operator-(const v2 &in);
internal v2 operator*(f32 A, v2 B);
internal v2 operator*(v2 B, f32 A);
internal v2 operator+(v2 A, v2 B);
internal v2 operator-(v2 A, v2 B);
internal v2& operator+=(v2& a, v2 b);
internal v2& operator-=(v2& a, v2 b);
internal v2& operator*=(v2& a, f32 b);
internal v2 binormal_to_normal(v2 x);

internal f32 dot(v2 a, v2 b);
internal f32 dot(v3 a, v3 b);
internal f32 dot(v4 a, v4 b);
internal f32 dot(Quaternion a, Quaternion b);

internal v3 cross(v3 a, v3 b);

internal v2 hadamard(v2 a, v2 b);
internal v3 hadamard(v3 a, v3 b);
internal v4 hadamard(v4 a, v4 b);

internal f64 fmod_cycling(f64 x, f64 y);
internal f32 fmod_cycling(f32 x, f32 y);

internal f32 sqlen(v2 v);
internal f32 sqlen(v3 v);

internal f32 invsqlen(v2 v);
internal f32 invsqlen(v3 v);
internal f32 invsqlen(v4 v);

internal f32 length(v2 A);
internal v2 normalize(v2 a);
internal v2 lerp(v2 a, f32 t, v2 b);
internal v3 V3(f32 x, f32 y, f32 z);
internal v3 V3(v2 xy, f32 z);
internal v3 V3(f32 a);
internal b32 operator == (v3 a, v3 b);
internal v3  operator - (const v3 &in);
internal v3  operator * (f32 A, v3 B);
internal v3  operator * (v3 B, f32 A);
internal v3  operator / (v3 a, f32 b);
internal v3& operator /= (v3& a, f32 b);
internal v3  operator + (v3 A, v3 B);
internal v3  operator - (v3 A, v3 B);
internal v3& operator += (v3& a, v3 b);
internal v3& operator -= (v3& a, v3 b);
internal v3& operator *= (v3& a, f32 b);
internal f32 length(v3 A);
internal v3 normalize(v3 a);
internal v3 lerp(v3 a, f32 t, v3 b);
internal f32 distance(v3 a, v3 b);
internal f32 distance(v2 a, v2 b);
internal f32 point_line_distance(v2 p, v2 a, v2 b);
internal v4 V4(f32 x);
internal v4 V4(f32 r, f32 g, f32 b, f32 a);
internal v4 V4(v2 rg, f32 b, f32 a);
internal v4 V4(v3 rgb, f32 a);
internal v4 operator * (v4 a, f32 b);
internal v4 lerp(v4 a, f32 t, v4 b);
internal Quaternion operator + (Quaternion a, Quaternion b);
internal Quaternion operator * (Quaternion a, Quaternion b);
internal Quaternion operator * (Quaternion a, f32 b);
internal Quaternion operator * (f32 b, Quaternion a);
internal Quaternion operator - (Quaternion in);
internal Quaternion nlerp(Quaternion a, f32 t, Quaternion b);
internal Quaternion slerp(Quaternion q1, f32 t, Quaternion q2);
internal m4x4 operator * (m4x4 a, m4x4 b);
internal v4 operator * (m4x4 m, v4 p);
internal m4x4 identity();
internal m4x4 x_rotation(f32 a);
internal m4x4 y_rotation(f32 a);
internal m4x4 z_rotation(f32 a);
internal m4x4 transpose(m4x4 m);
internal m4x4 inverse(m4x4 m);
internal m4x4 rows(v3 x, v3 y, v3 z);
internal m4x4 columns(v3 x, v3 y, v3 z);
internal m4x4 translate(m4x4 m, v3 t);
internal m4x4 quaternion_to_m4x4(Quaternion q);
internal Quaternion euler_to_quaternion(f32 roll, f32 pitch, f32 yaw);
internal m4x4 scale(m4x4 transform, v3 factor);
internal m4x4 scale(m4x4 transform, f32 factor);
internal m4x4 scale(f32 s);
internal m4x4 camera_transform(v3 x, v3 y, v3 z, v3 p);
internal v3 get_row(m4x4 M, u32 R);
internal v3 get_column(m4x4 M, u32 C);
internal Rect2 rect2_min_max(v2 min, v2 max);
internal Rect2 rect2_cen_half_dim(v2 cen, v2 h_dim);
internal Rect2 rect2_min_dim(v2 min, v2 dim);
internal Rect2 rect2_inv_inf();
internal Rect2 offset(Rect2 rect, v2 offset);
internal Rect2 add_radius_to(Rect2 rect, v2 radius);
internal m4x4 trs_to_transform(v3 translation, Quaternion rotation, v3 scaling);
internal Quaternion build_quaternion(v3 axis, f32 radian);
internal Quaternion rotate(Quaternion q0, v3 axis, f32 radian);
internal v3 project(v3 p, m4x4 view_proj);
internal v2 V2(v2u v);
internal m4x4 lookat(v3 eye, v3 center, v3 up_);
internal m4x4 view_transform(v3 position, Quaternion orientation);
internal m4x4 ortho(f32 min_x, f32 max_x, f32 min_y, f32 max_y, f32 min_z, f32 max_z);
internal f32 radian_from_degree(f32 d);

internal f32 normalize01(v2 range, f32 val);
internal b32 intersects(AABB2 box, v2 point);
internal b32 intersects(AABB2 a, AABB2 b);
internal AABB2 intersection(AABB2 a, AABB2 b);
internal AABB2 aabb2_infinite(void);

internal v2 to_ndc(v2 p, f32 w, f32 h);
internal v3 unproject(v3 position, m4x4 viewproj);
internal Ray3 ray_from_screen_position(v2 position, f32 screen_width, f32 screen_height, m4x4 viewproj);
