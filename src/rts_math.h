/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Seong Woo Lee $
   $Notice: (C) Copyright %s by Seong Woo Lee. All Rights Reserved. $
   ======================================================================== */

#define pi32                3.141592f
#define epsilon_f32         1.19209e-07f

union v2 {
    struct { f32 x, y; };
    f32 e[2];
};

union v2u {
    struct {u32 x, y;};
    struct {u32 w, h;};
    u32 e[2];
};

union v3 {
    struct {
        union {
            struct { f32 x, y; };
            v2 xy;
        };
        f32 z;
    };
    f32 e[3];
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
};

struct m4x4 
{
    f32 e[4][4];
};

struct Quaternion 
{
    f32 w, x, y, z;
};

struct Rect2 
{
    v2 min, max;
};

internal f32 absolute(f32 val);
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
internal f32 dot(v2 a, v2 b);
internal v2 hadamard(v2 A, v2 B);
internal v2 hadamard(v2 A, v2u B);
internal v2 binormal_to_normal(v2 x);
internal f32 length_square(v2 A);
internal f32 inv_length_square(v2 A);
internal f32 length(v2 A);
internal v2 normalize(v2 a);
internal v3 V3(f32 x, f32 y, f32 z);
internal v3 V3(v2 xy, f32 z);
internal v3 V3(f32 a);
internal b32 operator == (v3 a, v3 b);
internal v3 operator - (const v3 &in);
internal v3 operator * (f32 A, v3 B);
internal v3 operator * (v3 B, f32 A);
internal v3 operator / (v3 a, f32 b);
internal v3& operator /= (v3& a, f32 b);
internal v3 operator + (v3 A, v3 B);
internal v3 operator - (v3 A, v3 B);
internal v3& operator += (v3& a, v3 b);
internal v3& operator -= (v3& a, v3 b);
internal v3& operator *= (v3& a, f32 b);
internal f32 dot(v3 a, v3 b);
internal v3 hadamard(v3 A, v3 B);
internal v3 cross(v3 A, v3 B);
internal f32 length_square(v3 A);
internal f32 length(v3 A);
internal v3 normalize(v3 a);
internal v3 lerp(v3 a, f32 t, v3 b);
internal f32 distance(v3 a, v3 b);
internal f32 distance(v2 a, v2 b);
internal f32 point_line_distance(v2 p, v2 a, v2 b);
internal v3 linear_to_srgb(v3 c);
internal v3 srgb_to_linear(v3 c);
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
internal f32 dot(Quaternion a, Quaternion b);
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
internal b32 in_rect(Rect2 rect, v2 p);
internal v2 get_dim(Rect2 rect);
internal m4x4 trs_to_transform(v3 translation, Quaternion rotation, v3 scaling);
internal Quaternion build_quaternion(v3 axis, f32 radian);
internal Quaternion rotate(Quaternion q0, v3 axis, f32 radian);
internal v3 project(v3 p, m4x4 view_proj);
internal v2 V2(v2u v);
internal m4x4 lookat(v3 eye, v3 center, v3 up_);
internal m4x4 view_transform(v3 position, Quaternion orientation);
internal m4x4 ortho(f32 min_x, f32 max_x, f32 min_y, f32 max_y, f32 min_z, f32 max_z);
internal f32 degrees_to_radian(f32 x);
