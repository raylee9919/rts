// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RTS_MATH_H
#define RTS_MATH_H

#include "basic/core.h"


/*
 sin/cos usually have high precision, but they still differ across CRT implementations.
 So it is better for me to roll my own to guarantee consistency across platforms.
 sqrt nowadays is a single CPU instruction, there's nothing else to implement.
 You just use correct builtin/intrinsic from compiler.
*/

const read_only f32 PI    = 3.1415927f;
const read_only f64 PI64  = 3.141592653589793;
const read_only f32 TAU   = 6.283185f;
const read_only f64 TAU64 = 6.28318530717958648;

// @Robustness
#define _FAR_Z  ( 1.f)
#define _NEAR_Z (-1.f)


union vec2 
{
  struct { f32 x, y; };
  f32 e[2];

  force_inline vec2() = default;
  force_inline vec2(f32 x_, f32 y_) : x(x_), y(y_) {};
  force_inline explicit vec2(f32 f) : x( f), y( f) {};
};

struct vec3 
{
  union {
    struct {
      union {
        vec2 xy;
        struct { f32 x, y; };
      };
      f32 z;
    };
    f32 e[3];
  };

  force_inline vec3() = default;
  force_inline vec3(f32 x_, f32 y_, f32 z_)  : x(x_), y(y_), z(z_) {};
  force_inline vec3(vec2 xy_, f32 z_)        : xy(xy_), z(z_)      {};
  force_inline explicit vec3(f32 f)          : x( f), y( f), z( f) {};
};

union vec4 
{
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

  force_inline vec4() = default;
  force_inline vec4(f32 f1, f32 f2, f32 f3, f32 f4) : x(f1), y(f2), z(f3), w(f4) {};
  force_inline explicit vec4(f32 f)                 : x( f), y( f), z( f), w( f) {};
  force_inline vec4(vec3 xyz, f32 w)                : x(xyz.x), y(xyz.y), z(xyz.z), w(w) {};
  force_inline vec4(vec2 xy, f32 z, f32 w)          : x(xy.x), y(xy.y), z(z), w(w) {};
};

union m4x4 
{
  f32 f[16];
  f32 e[4][4];
  struct {
    f32 _11, _12, _13, _14; 
    f32 _21, _22, _23, _24; 
    f32 _31, _32, _33, _34; 
    f32 _41, _42, _43, _44; 
  };
  vec4 rows[4];
};

union m3x4 
{
  struct {
    f32 _11, _12, _13, _14; 
    f32 _21, _22, _23, _24; 
    f32 _31, _32, _33, _34; 
  };
};

union Quaternion 
{
  struct { f32 w, x, y, z; };
  f32 f[4];
};

union Rect {
  struct {
    vec2 min;
    vec2 max;
  };
};

union AABB {
  struct {
    vec3 min;
    vec3 max;
  };
  struct { f32 x1, y1, z1, x2, y2, z2; };
};

struct AABBV {
  __m128 lo; // minX, minY, minZ, dummy
  __m128 hi; // maxX, maxY, maxZ, dummy
};


struct Ray {
  vec3 origin;
  vec3 direction;
};


// Trigonometry
f32        m_sin(f32 f);
f32        m_cos(f32 f);

// Absolute
f32        m_abs(f32 f);
f64        m_abs(f64 d);

// Exponent
f32        m_exp(f32 f);


//// Square roots
f32        m_sqrt(f32 f);
f32        m_rsqrt(f32 f);


//// Easing functions
f32        
lerp(f32 a, f32 b, f32 t);

f32        
smoothstep(f32 min, f32 max, f32 x);


//
// Vector2
//
vec2       operator  - (vec2& in);
vec2       operator  * (f32 f,  vec2 v);
vec2       operator  * (vec2  v, f32 f);
vec2       operator  / (vec2  v, f32 f);
vec2       operator  + (vec2  l,  vec2 r);
vec2       operator  - (vec2  l,  vec2 r);
vec2&      operator += (vec2& l,  vec2 r);
vec2&      operator -= (vec2& l,  vec2 r);
vec2&      operator *= (vec2& v, f32 f);
vec2       operator  * (vec2  l,  vec2 r); // Hadamard product. Shader convention.
vec2       normalize(vec2 a);
f32        length(vec2 A);
f32        distance(vec2 a, vec2 b);
vec2       lerp(vec2 a, vec2 b, f32 t);
f32        sqlen(vec2 v);
f32        invsqlen(vec2 v);
f32        dot(vec2 a, vec2 b);

//
// @Todo: Cleanup below.
//

f32            dot(vec3 a, vec3 b);
f32            dot(Quaternion a, Quaternion b);

vec3           cross(vec3 a, vec3 b);

vec3           hadamard(vec3 a, vec3 b);

f64            fmod_cycling(f64 x, f64 y);
f32            fmod_cycling(f32 x, f32 y);

f32            sqlen(vec3 v);

f32            invsqlen(vec3 v);
f32            invsqlen(vec4 v);

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



//
// Vector4
//
vec4           
operator + (vec4 a, vec4 b);

vec4           
operator - (vec4 a, vec4 b);

vec4           
operator * (vec4 v, f32 f);

vec4           
operator * (f32 a, vec4 v);

vec4           
operator / (vec4 v, f32 f);

f32            
dot(vec4 a, vec4 b);

vec4           
operator * (vec4 a, vec4 b); // Hadamard

vec4           
lerp(vec4 a, vec4 b, f32 t);


// 
// Quaternion
//
Quaternion 
make_quaternion(f32 w, f32 x, f32 y, f32 z);

Quaternion     
operator + (Quaternion l, Quaternion r);

Quaternion     
operator - (Quaternion l, Quaternion r);

Quaternion     
operator * (Quaternion l, Quaternion r);

Quaternion     
operator * (Quaternion q, f32 f);

Quaternion     
operator * (f32 f, Quaternion q);

Quaternion     
operator - (Quaternion q);

Quaternion     
normalize(Quaternion q);

Quaternion     
nlerp(Quaternion a, Quaternion b, f32 t);

Quaternion     
slerp(Quaternion q1, Quaternion q2, f32 t);

force_inline Quaternion     
nlerp_shortest(Quaternion a, Quaternion b, f32 t)
{
  if (dot(a, b) < 0)  b = -b;
  return nlerp(a, b, t);
}

force_inline Quaternion     
slerp_shortest(Quaternion a, Quaternion b, f32 t)
{
  if (dot(a, b) < 0)  b = -b;
  return slerp(a, b, t);
}


//
// Matrix4x4
//   The math library follows *row-major*, *column-vector*.
//

// @Cleanup: Just use pointer...
m4x4           
operator * (m4x4& l, m4x4& r);

vec4           
operator * (m4x4 m, vec4 v);

m4x4 
identity();

m4x4 
x_rotation(f32 a);

m4x4 
y_rotation(f32 a);

m4x4 
z_rotation(f32 a);

m4x4 
transpose(const m4x4& m);

m4x4 
inverse(const m4x4& m);

m4x4           
m4x4_translate(f32 x, f32 y, f32 z);

m4x4           
m4x4_translate(vec3 t);


m4x4           to_m4x4(Quaternion q);
Quaternion     euler_to_quaternion(f32 roll, f32 pitch, f32 yaw);
m4x4           m4x4_scale(f32 x, f32 y, f32 z);
m4x4           to_m4x4(vec3 translation, Quaternion rotation, vec3 scale);
Quaternion     build_quaternion(vec3 axis, f32 radian);
Quaternion     rotate(Quaternion q0, vec3 axis, f32 radian);



f32            
radian_from_degree(f32 d);


/*
 Graphics

 I like to view the process of building look-at matrix as determining the
 local axes of a camera placed in the world. The axes are the camera's own
 language, into which the objects' positions, expressed in the world's
 language are translated.

 The problem is that axes can be arbitrary. Just imagine three orthogonal
 axes rotating rotating wildly around the camera. Thus, we must impose some
 constraints.

 As most, if not all, graphics APIs' viewport is X-right and Y-up, these will
 be the premises in the API. Of course, I provide additional functionality to
 customize them, but IMO, the extra verbosity isn't worth it.

 Now, all we have to determine is Z. Does it point "foward" or "backward"?
 In other words, is it left-handed or right-handed? This is only variant exposed by the API.
*/
m4x4 
look_to_lh(vec3 from, vec3 to, vec3 up);

m4x4 
look_to_rh(vec3 from, vec3 to, vec3 up);

m4x4 
look_at_lh(vec3 from, vec3 at, vec3 up);

m4x4 
look_at_rh(vec3 from, vec3 at, vec3 up);


/*
 For perspective projection as well, we impose the following premises: [0,1]
 for NDC depth, X-right and Y-up. The projection maps 'near_z' to 0 and 'far_z' to 1. 
 
 LH and RH variants specify the handedness of the input space. The 'near_z' and 
 'far_z' parameters are distances, and are therefore unsigned.
*/
m4x4 
persp_fov_lh(f32 fov, f32 aspect_ratio, f32 near_z, f32 far_z);

m4x4 
persp_fov_rh(f32 fov, f32 aspect_ratio, f32 near_z, f32 far_z);

m4x4 
persp_asymmetric_lh(f32 xmin, f32 xmax, f32 ymin, f32 ymax, f32 near_z, f32 far_z);

m4x4 
persp_asymmetric_rh(f32 xmin, f32 xmax, f32 ymin, f32 ymax, f32 near_z, f32 far_z);


f32           m_tan(f32 f);
f32           m_atan(f32 f);
f32           m_atan2(f32 a, f32 b);
m4x4&         operator += (m4x4& l, m4x4 r);

u32  
pack_rgba(vec4 rgba);

vec4 
unpack_rgba(u32 rgba);

Rect
rect_union(Rect a, Rect b);

vec2
rect_center(Rect r);


//
// AABBV (SIMD version of AABB)
//
AABBV
aabbv_make(vec3 lo, vec3 hi);

AABBV
aabbv_make(f32 lo_x, f32 lo_y, f32 lo_z, 
           f32 hi_x, f32 hi_y, f32 hi_z);

AABBV
aabbv_from_aabb(AABB box);

AABB
aabb_from_aabbv(AABBV box);

AABBV
aabbv_union(AABBV a, AABBV b);

void
aabbv_expand(AABBV *box, f32 extent);

AABBV
aabbv_translate(AABBV box, vec3 t);

f32
aabbv_surface(AABBV box);

b32 
aabbv_contains(AABBV *out, AABBV *in);

//
// Ray
//
force_inline Ray 
make_ray(vec3 origin, vec3 direction)
{
  Ray r = { origin, direction };
  return r;
}


//
// Collision, Intersection
//
b32 
ray_aabb_intersect(Ray *ray, f32 *tmin_out, AABB *box);

b32 
view_frustum_aabb_intersect(m4x4 *view_proj, AABB *box);





#endif // RTS_MATH_H
