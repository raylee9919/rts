// Copyright Seong Woo Lee. All Rights Reserved.

#include "math/math.h"


//
// SIMD
//
static f32
dotv(f32 *a, f32 *b)
{
  f32 f;
  __m128 av = _mm_loadu_ps(a);
  __m128 bv = _mm_loadu_ps(b);
  __m128 p = _mm_mul_ps(av, bv);
  __m128 q = _mm_shuffle_ps(p, p, _MM_SHUFFLE(2, 3, 0, 1));
  p = _mm_add_ps(p, q);
  q = _mm_shuffle_ps(p, p, _MM_SHUFFLE(0, 1, 2, 3));
  p = _mm_add_ps(p, q);
  _mm_store_ss(&f, p);
  return f;
}

static f32
dotv(__m128 av, __m128 bv)
{
  f32 f;
  __m128 p = _mm_mul_ps(av, bv);
  __m128 q = _mm_shuffle_ps(p, p, _MM_SHUFFLE(2, 3, 0, 1));
  p = _mm_add_ps(p, q);
  q = _mm_shuffle_ps(p, p, _MM_SHUFFLE(0, 1, 2, 3));
  p = _mm_add_ps(p, q);
  _mm_store_ss(&f, p);
  return f;
}

// Trigonometry
//
f32 m_sin(f32 f) {
  return sinf(f);
}

f32 m_cos(f32 f) {
  return cosf(f);
}

f32 m_tan(f32 f) {
  return tanf(f);
}

f32 m_atan(f32 f) {
  return atanf(f);
}

f32 m_atan2(f32 a, f32 b) {
  return atan2(a, b);
}

// Exponent
//
f32 
m_exp(f32 f)
{
  return expf(f);
}


// Absolute
//
f32 m_abs(f32 f) { return f > 0.f ? f : -f; }
f64 m_abs(f64 d) { return d > 0.f ? d : -d; }


// Square roots
//
f32 m_sqrt(f32 f) {
  // Use '_mm_cvtss_f32' instead of '_mm_store_ss' to extract a float.
  return _mm_cvtss_f32(_mm_sqrt_ss(_mm_set_ss(f)));
}

f32 m_rsqrt(f32 f) {
  /*
   @Study: 1. 16.17 FSQRT SQRTSS (https://www.agner.org/optimize/optimizing_assembly.pdf)
           2. Newtonian Iteration (https://stackoverflow.com/questions/14752399/newton-raphson-with-sse2-can-someone-explain-me-these-3-lines)
   @Todo: afaik, '_mm_rsqrt' does not produce deterministic result?
  */
  return _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(f)));
}


// Easing functions
//
f32 lerp(f32 a, f32 b, f32 t) {
  return a + (b - a) * t;
}

f32 smoothstep(f32 min, f32 max, f32 x) {
  x = x / (max - min);
  return x * x * (3.f - 2.f*x);
}



// Vector2
//
vec2 operator - (vec2& in) {
  vec2 V;
  V.x = -in.x;
  V.y = -in.y;
  return V;
}

vec2 operator * (f32 f, vec2 v) {
  v.x *= f;
  v.y *= f;
  return v;
}

vec2 operator * (vec2 v, f32 f) {
  v.x *= f;
  v.y *= f;
  return v;
}

vec2 operator / (vec2 v, f32 f) {
  f32 r = 1.f / f;
  return vec2{v.x*r, v.y*r};
}

vec2 operator + (vec2 a, vec2 b) {
  vec2 v;
  v.x = a.x + b.x;
  v.y = a.y + b.y;
  return v;
}

vec2 operator - (vec2 a, vec2 b) {
  vec2 v;
  v.x = a.x - b.x;
  v.y = a.y - b.y;
  return v;
}

vec2& operator += (vec2& a, vec2 b) {
  a.x += b.x;
  a.y += b.y;
  return a;
}

vec2& operator-=(vec2& a, vec2 b) {
  a.x -= b.x;
  a.y -= b.y;
  return a;
}

vec2& operator *= (vec2& a, f32 b) {
  a.x *= b;
  a.y *= b;
  return a;
}

vec2 operator * (vec2 l, vec2 r) {
  vec2 v = { l.x * r.x, l.y * r.y };
  return v;
}

f64 fmod_cycling(f64 x, f64 y) 
{
  R_ASSERT( y != 0 );
  f64 remainder = x - (floor(x/y) * y);
  return remainder;
}

f32 fmod_cycling(f32 x, f32 y) 
{
  return (f32)fmod_cycling((f64)x, (f64)y);
}

f32 sqlen(vec2 v) {
  return dot(v,v);
}

f32 sqlen(vec3 v) {
  return dot(v,v);
}

f32 invsqlen(vec2 v) {
  f32 result = 1.f / dot(v, v);
  return result;
}

f32 invsqlen(vec3 v) {
  f32 result = 1.f / dot(v, v);
  return result;
}

f32 invsqlen(vec4 v) {
  f32 result = 1.f / dot(v, v);
  return result;
}

f32 length(vec2 v) {
  f32 len = m_sqrt(sqlen(v));
  return len;
}

vec2 lerp(vec2 a, vec2 b, f32 t) {
  vec2 v;
  v.x = lerp(a.x, b.x, t);
  v.y = lerp(a.y, b.y, t);
  return v;
}

vec2 normalize(vec2 v) {
  f32 d = m_rsqrt(v.x * v.x + v.y * v.y);
  v.x *= d;
  v.y *= d;
  return v;
}


vec3 operator - (const vec3 &in) {
  vec3 V;
  V.x = -in.x;
  V.y = -in.y;
  V.z = -in.z;
  return V;
}

vec3 operator * (f32 f, vec3 v) {
  v.x = f * v.x;
  v.y = f * v.y;
  v.z = f * v.z;
  return v;
}

vec3 operator * (vec3 v, f32 f) {
  return f * v;
}

vec3 operator / (vec3 v, f32 f) {
  f32 r = 1.f / f;
  v.x *= r;
  v.y *= r;
  v.z *= r;
  return v;
}

vec3& operator /= (vec3& a, f32 b) {
  f32 c = (1.0f / b);
  a.x *= c;
  a.y *= c;
  a.z *= c;
  return a;
}

vec3 operator + (vec3 l, vec3 r) {
  vec3 v;
  v.x = l.x + r.x;
  v.y = l.y + r.y;
  v.z = l.z + r.z;
  return v;
}

vec3 operator - (vec3 l, vec3 r) {
  vec3 v;
  v.x = l.x - r.x;
  v.y = l.y - r.y;
  v.z = l.z - r.z;
  return v;
}

vec3&
operator += (vec3& a, vec3 b) 
{
  a.x += b.x;
  a.y += b.y;
  a.z += b.z;

  return a;
}

vec3&
operator -= (vec3& a, vec3 b) 
{
  a.x -= b.x;
  a.y -= b.y;
  a.z -= b.z;

  return a;
}

vec3&
operator *= (vec3& a, f32 b) 
{
  a.x *= b;
  a.y *= b;
  a.z *= b;
  return a;
}

f32 
dot(vec2 l, vec2 r) 
{
  f32 f = l.x*r.x + l.y*r.y;
  return f;
}

f32 
dot(vec3 l, vec3 r) 
{
  f32 f = l.x*r.x + l.y*r.y + l.z*r.z;
  return f;
}

vec3 
cross(vec3 a, vec3 b) 
{
  vec3 v;
  v.x = (a.y*b.z) - (b.y*a.z);
  v.y = (a.z*b.x) - (b.z*a.x);
  v.z = (a.x*b.y) - (b.x*a.y);
  return v;
}

vec3 
hadamard(vec3 a, vec3 b) 
{
  return vec3{ a.x*b.x, a.y*b.y, a.z*b.z };
}

f32 length(vec3 v) 
{
  f32 len = m_sqrt(v.x*v.x + v.y*v.y + v.z*v.z);
  return len;
}

b32 is_zero(vec3 v) {
  return v.x==0.f && v.y==0.f && v.z==0.f;
}

b32 is_inf(vec3 v) {
  return isinf(v.x) || isinf(v.y) || isinf(v.z);
}

vec3 normalize(vec3 v) {
  f32 d = m_rsqrt(v.x*v.x + v.y*v.y + v.z*v.z);
  v.x *= d;
  v.y *= d;
  v.z *= d;
  return v;
}

vec3 lerp(vec3 a, vec3 b, f32 t) {
  vec3 v;
  v.x = lerp(a.x, b.x, t);
  v.y = lerp(a.y, b.y, t);
  v.z = lerp(a.z, b.z, t);
  return v;
}

f32 distance(vec3 a, vec3 b) {
  f32 dx = a.x - b.x;
  f32 dy = a.y - b.y;
  f32 dz = a.z - b.z;
  return m_sqrt(dx*dx + dy*dy + dz*dz);
}

f32 distance(vec2 a, vec2 b) {
  f32 dx = a.x - b.x;
  f32 dy = a.y - b.y;
  return m_sqrt(dx*dx + dy*dy);
}

//
// Vector4 
//
vec4 
operator + (vec4 a, vec4 b) 
{
  // SSE
  vec4 v;
  __m128 av = _mm_loadu_ps(&a.x);
  __m128 bv = _mm_loadu_ps(&b.x);
  __m128 p  = _mm_add_ps(av, bv);
  _mm_storeu_ps(&v.x, p);
  return v;
}

vec4 
operator - (vec4 a, vec4 b)
{
  // SSE
  vec4 v;
  __m128 av = _mm_loadu_ps(&a.x);
  __m128 bv = _mm_loadu_ps(&b.x);
  __m128 p  = _mm_sub_ps(av, bv);
  _mm_storeu_ps(&v.x, p);
  return v;
}

vec4 
operator * (vec4 v, f32 f) 
{
  // SSE
  __m128 vv = _mm_loadu_ps(&v.x);
  __m128 fv = _mm_set1_ps(f);
  __m128 p  = _mm_mul_ps(vv, fv);
  vec4 v_;
  _mm_storeu_ps(&v_.x, p);
  return v_;
}

vec4 
operator * (f32 f, vec4 v) 
{
  return v * f;
}

vec4           
operator / (vec4 v, f32 f)
{
  // SSE
  __m128 vv = _mm_loadu_ps(&v.x);
  __m128 fv = _mm_set1_ps(f);
  __m128 p  = _mm_div_ps(vv, fv);
  vec4 v_;
  _mm_storeu_ps(&v_.x, p);
  return v_;
}

f32 
dot(vec4 a, vec4 b) 
{
  return dotv(&a.x, &b.x);
}

vec4           
operator * (vec4 a, vec4 b)
{
  // SSE
  vec4 v;
  __m128 av = _mm_loadu_ps(&a.x);
  __m128 bv = _mm_loadu_ps(&b.x);
  __m128 m  = _mm_mul_ps(av, bv);
  _mm_storeu_ps(&v.x, m);
  return v;
}

vec4 
lerp(vec4 a, vec4 b, f32 t) 
{
  // SSE
  __m128 av = _mm_loadu_ps(&a.x);
  __m128 bv = _mm_loadu_ps(&b.x);
  __m128 tv = _mm_set1_ps(t);
  __m128 dv = _mm_sub_ps(bv, av);
  __m128 l  = _mm_add_ps(av, _mm_mul_ps(tv, dv));
  vec4 v;
  _mm_storeu_ps(&v.x, l);
  return v;
}

//
// Quaternion
//
Quaternion 
make_quaternion(f32 w, f32 x, f32 y, f32 z)
{
  Quaternion q;
  _mm_storeu_ps(q.f, _mm_setr_ps(w, x, y, z));
  return q;
}

Quaternion 
operator + (Quaternion l, Quaternion r) 
{
  Quaternion q;
  _mm_storeu_ps(q.f, _mm_add_ps(_mm_loadu_ps(l.f), _mm_loadu_ps(r.f)));
  return q;
}

Quaternion 
operator - (Quaternion l, Quaternion r) 
{
  Quaternion q;
  _mm_storeu_ps(q.f, _mm_sub_ps(_mm_loadu_ps(l.f), _mm_loadu_ps(r.f)));
  return q;
}

Quaternion 
operator * (Quaternion a, Quaternion b)
{
  Quaternion q;
  q.w = (a.w * b.w) - (a.x * b.x) - (a.y * b.y) - (a.z * b.z); 
  q.x = (a.w * b.x) + (a.x * b.w) + (a.y * b.z) - (a.z * b.y); 
  q.y = (a.w * b.y) + (a.y * b.w) + (a.z * b.x) - (a.x * b.z); 
  q.z = (a.w * b.z) + (a.z * b.w) + (a.x * b.y) - (a.y * b.x); 
  return q;
}

Quaternion 
operator * (Quaternion q, f32 f) 
{
  // SSE
  __m128 qv = _mm_loadu_ps(q.f);
  __m128 fv = _mm_set1_ps(f);
  __m128 mv = _mm_mul_ps(qv, fv);
  Quaternion q_;
  _mm_storeu_ps(q_.f, mv);
  return q_;
}

Quaternion 
operator * (f32 f, Quaternion q) 
{
  return q * f;
}

Quaternion 
operator - (Quaternion q) 
{
  Quaternion q_;
  _mm_storeu_ps(q_.f, _mm_mul_ps(_mm_set1_ps(-1.f), _mm_loadu_ps(q.f)));
  return q_;
}

Quaternion 
normalize(Quaternion q) 
{
  // @Todo: Better?
  f32 rcp = m_rsqrt(dotv(q.f, q.f));
  __m128 rcp_v = _mm_set1_ps(rcp);
  Quaternion q_;
  _mm_storeu_ps(q_.f, _mm_mul_ps(_mm_loadu_ps(q.f), rcp_v));
  return q_;
}

f32 
dot(Quaternion a, Quaternion b) 
{
  return dotv(a.f, b.f);
}

Quaternion 
nlerp(Quaternion a, Quaternion b, f32 t) 
{
  // @Todo: Should I return normalized result?
  __m128 av = _mm_loadu_ps(a.f);
  __m128 bv = _mm_loadu_ps(b.f);
  __m128 tv = _mm_set1_ps(t);
  __m128 dv = _mm_sub_ps(bv, av);
  Quaternion q;
  _mm_storeu_ps(q.f, _mm_add_ps(av, _mm_mul_ps(tv, dv)));
  return q;
}

Quaternion 
slerp(Quaternion q1, Quaternion q2, f32 t) 
{
  // @Todo: SIMD
  f32 cosom = q1.x * q2.x + q1.y * q2.y + q1.z * q2.z + q1.w * q2.w;
  Quaternion q3 = q2;
  if (cosom < 0.0f) {
    cosom = -cosom;
    q3.w = -q3.w;
    q3.x = -q3.x;
    q3.y = -q3.y;
    q3.z = -q3.z;
  }

  f32 sclp, sclq;
  f32 threshold = 1e-8f;

  if (1.0f - cosom > threshold) {
    f32 omega, sinom;
    omega = acosf(cosom);
    sinom = m_sin(omega);
    sclp  = m_sin((1.0f - t) * omega) / sinom;
    sclq  = m_sin(t * omega) / sinom;
  } else {
    sclp = 1.0f - t;
    sclq = t;
  }

  Quaternion q;
  q.w = sclp * q1.w + sclq * q3.w;
  q.x = sclp * q1.x + sclq * q3.x;
  q.y = sclp * q1.y + sclq * q3.y;
  q.z = sclp * q1.z + sclq * q3.z;
  return q;
}

//
// Matrix4x4
//
static __m128 
lincomb(__m128 a, m4x4 b)
{
  __m128 m;
  m = _mm_mul_ps(_mm_shuffle_ps(a, a, 0x00), _mm_loadu_ps(&b.rows[0].x));
  m = _mm_add_ps(m, _mm_mul_ps(_mm_shuffle_ps(a, a, 0x55), _mm_loadu_ps(&b.rows[1].x)));
  m = _mm_add_ps(m, _mm_mul_ps(_mm_shuffle_ps(a, a, 0xaa), _mm_loadu_ps(&b.rows[2].x)));
  m = _mm_add_ps(m, _mm_mul_ps(_mm_shuffle_ps(a, a, 0xff), _mm_loadu_ps(&b.rows[3].x)));
  return m;
}

m4x4 
operator * (m4x4 l, m4x4 r) 
{
  m4x4 m;
  __m128 out1x = lincomb(_mm_loadu_ps(&l.rows[0].x), r);
  __m128 out2x = lincomb(_mm_loadu_ps(&l.rows[1].x), r);
  __m128 out3x = lincomb(_mm_loadu_ps(&l.rows[2].x), r);
  __m128 out4x = lincomb(_mm_loadu_ps(&l.rows[3].x), r);

  _mm_storeu_ps(&m.rows[0].x, out1x);
  _mm_storeu_ps(&m.rows[1].x, out2x);
  _mm_storeu_ps(&m.rows[2].x, out3x);
  _mm_storeu_ps(&m.rows[3].x, out4x);

  return m;
}

vec4 
operator * (m4x4 m, vec4 v) 
{
  vec4 v_;
  _mm_storeu_ps(v_.e, _mm_setr_ps(dotv(m.rows[0].e, v.e), 
                                  dotv(m.rows[1].e, v.e), 
                                  dotv(m.rows[2].e, v.e), 
                                  dotv(m.rows[3].e, v.e)));
  return v_;
}

m4x4& operator += (m4x4& l, m4x4 r) 
{
  l.rows[0] = l.rows[0] + r.rows[0];
  l.rows[1] = l.rows[1] + r.rows[1];
  l.rows[2] = l.rows[2] + r.rows[2];
  l.rows[3] = l.rows[3] + r.rows[3];
  return l;
}

m4x4 
identity() 
{
  m4x4 m;
  _mm_storeu_ps(&m.rows[0].x, _mm_setr_ps(1.f, 0.f, 0.f, 0.f));
  _mm_storeu_ps(&m.rows[1].x, _mm_setr_ps(0.f, 1.f, 0.f, 0.f));
  _mm_storeu_ps(&m.rows[2].x, _mm_setr_ps(0.f, 0.f, 1.f, 0.f));
  _mm_storeu_ps(&m.rows[3].x, _mm_setr_ps(0.f, 0.f, 0.f, 1.f));
  return m;
}

m4x4 
x_rotation(f32 a) 
{
  f32 c = m_cos(a);
  f32 s = m_sin(a);
  m4x4 m;
  _mm_storeu_ps(&m.rows[0].x, _mm_setr_ps( 1,  0,  0,  0));
  _mm_storeu_ps(&m.rows[1].x, _mm_setr_ps( 0,  c, -s,  0));
  _mm_storeu_ps(&m.rows[2].x, _mm_setr_ps( 0,  s,  c,  0));
  _mm_storeu_ps(&m.rows[3].x, _mm_setr_ps( 0,  0,  0,  1));
  return m;
}

m4x4 
y_rotation(f32 a) 
{
  f32 c = m_cos(a);
  f32 s = m_sin(a);
  m4x4 m;
  _mm_storeu_ps(&m.rows[0].x, _mm_setr_ps( c,  0,  s,  0));
  _mm_storeu_ps(&m.rows[1].x, _mm_setr_ps( 0,  1,  0,  0));
  _mm_storeu_ps(&m.rows[2].x, _mm_setr_ps(-s,  0,  c,  0));
  _mm_storeu_ps(&m.rows[3].x, _mm_setr_ps( 0,  0,  0,  1));
  return m;
}

m4x4 
z_rotation(f32 a) 
{
  f32 c = m_cos(a);
  f32 s = m_sin(a);
  m4x4 m;
  _mm_storeu_ps(&m.rows[0].x, _mm_setr_ps(c, -s,  0,  0));
  _mm_storeu_ps(&m.rows[1].x, _mm_setr_ps(s,  c,  0,  0));
  _mm_storeu_ps(&m.rows[2].x, _mm_setr_ps(0,  0,  1,  0));
  _mm_storeu_ps(&m.rows[3].x, _mm_setr_ps(0,  0,  0,  1));
  return m;
}

m4x4 
transpose(m4x4 m) 
{
  // @Todo: SIMD
  m4x4 n;

  for (int i = 0; i < 4; ++i)
    for (int j = 0; j < 4; ++j)
      n.e[i][j] = m.e[j][i];

  return n;
}

m4x4 
inverse(m4x4 m) 
{
  // @Cleanup
  f32 A2323 = m.e[2][2] * m.e[3][3] - m.e[2][3] * m.e[3][2];
  f32 A1323 = m.e[2][1] * m.e[3][3] - m.e[2][3] * m.e[3][1];
  f32 A1223 = m.e[2][1] * m.e[3][2] - m.e[2][2] * m.e[3][1];
  f32 A0323 = m.e[2][0] * m.e[3][3] - m.e[2][3] * m.e[3][0];
  f32 A0223 = m.e[2][0] * m.e[3][2] - m.e[2][2] * m.e[3][0];
  f32 A0123 = m.e[2][0] * m.e[3][1] - m.e[2][1] * m.e[3][0];
  f32 A2313 = m.e[1][2] * m.e[3][3] - m.e[1][3] * m.e[3][2];
  f32 A1313 = m.e[1][1] * m.e[3][3] - m.e[1][3] * m.e[3][1];
  f32 A1213 = m.e[1][1] * m.e[3][2] - m.e[1][2] * m.e[3][1];
  f32 A2312 = m.e[1][2] * m.e[2][3] - m.e[1][3] * m.e[2][2];
  f32 A1312 = m.e[1][1] * m.e[2][3] - m.e[1][3] * m.e[2][1];
  f32 A1212 = m.e[1][1] * m.e[2][2] - m.e[1][2] * m.e[2][1];
  f32 A0313 = m.e[1][0] * m.e[3][3] - m.e[1][3] * m.e[3][0];
  f32 A0213 = m.e[1][0] * m.e[3][2] - m.e[1][2] * m.e[3][0];
  f32 A0312 = m.e[1][0] * m.e[2][3] - m.e[1][3] * m.e[2][0];
  f32 A0212 = m.e[1][0] * m.e[2][2] - m.e[1][2] * m.e[2][0];
  f32 A0113 = m.e[1][0] * m.e[3][1] - m.e[1][1] * m.e[3][0];
  f32 A0112 = m.e[1][0] * m.e[2][1] - m.e[1][1] * m.e[2][0];

  f32 det = (m.e[0][0] * ( m.e[1][1] * A2323 - m.e[1][2] * A1323 + m.e[1][3] * A1223 ) - 
             m.e[0][1] * ( m.e[1][0] * A2323 - m.e[1][2] * A0323 + m.e[1][3] * A0223 ) +
             m.e[0][2] * ( m.e[1][0] * A1323 - m.e[1][1] * A0323 + m.e[1][3] * A0123 ) -
             m.e[0][3] * ( m.e[1][0] * A1223 - m.e[1][1] * A0223 + m.e[1][2] * A0123 ));
  det = 1.f / det;

  m4x4 result = m4x4{{
    det *   ( m.e[1][1] * A2323 - m.e[1][2] * A1323 + m.e[1][3] * A1223 ),
    det * - ( m.e[0][1] * A2323 - m.e[0][2] * A1323 + m.e[0][3] * A1223 ),
    det *   ( m.e[0][1] * A2313 - m.e[0][2] * A1313 + m.e[0][3] * A1213 ),
    det * - ( m.e[0][1] * A2312 - m.e[0][2] * A1312 + m.e[0][3] * A1212 ),
    det * - ( m.e[1][0] * A2323 - m.e[1][2] * A0323 + m.e[1][3] * A0223 ),
    det *   ( m.e[0][0] * A2323 - m.e[0][2] * A0323 + m.e[0][3] * A0223 ),
    det * - ( m.e[0][0] * A2313 - m.e[0][2] * A0313 + m.e[0][3] * A0213 ),
    det *   ( m.e[0][0] * A2312 - m.e[0][2] * A0312 + m.e[0][3] * A0212 ),
    det *   ( m.e[1][0] * A1323 - m.e[1][1] * A0323 + m.e[1][3] * A0123 ),
    det * - ( m.e[0][0] * A1323 - m.e[0][1] * A0323 + m.e[0][3] * A0123 ),
    det *   ( m.e[0][0] * A1313 - m.e[0][1] * A0313 + m.e[0][3] * A0113 ),
    det * - ( m.e[0][0] * A1312 - m.e[0][1] * A0312 + m.e[0][3] * A0112 ),
    det * - ( m.e[1][0] * A1223 - m.e[1][1] * A0223 + m.e[1][2] * A0123 ),
    det *   ( m.e[0][0] * A1223 - m.e[0][1] * A0223 + m.e[0][2] * A0123 ),
    det * - ( m.e[0][0] * A1213 - m.e[0][1] * A0213 + m.e[0][2] * A0113 ),
    det *   ( m.e[0][0] * A1212 - m.e[0][1] * A0212 + m.e[0][2] * A0112 ),
  }};

  return result;
}

m4x4 
m4x4_translate(f32 x, f32 y, f32 z) 
{
  // SSE
  m4x4 m;
  _mm_storeu_ps(&m.rows[0].x, _mm_setr_ps(1.f, 0.f, 0.f,   x));
  _mm_storeu_ps(&m.rows[1].x, _mm_setr_ps(0.f, 1.f, 0.f,   y));
  _mm_storeu_ps(&m.rows[2].x, _mm_setr_ps(0.f, 0.f, 1.f,   z));
  _mm_storeu_ps(&m.rows[3].x, _mm_setr_ps(0.f, 0.f, 0.f, 1.f));
  return m;
}

m4x4 
m4x4_translate(vec3 t) 
{
  // SSE
  m4x4 m;
  _mm_storeu_ps(&m.rows[0].x, _mm_setr_ps(1.f, 0.f, 0.f, t.x));
  _mm_storeu_ps(&m.rows[1].x, _mm_setr_ps(0.f, 1.f, 0.f, t.y));
  _mm_storeu_ps(&m.rows[2].x, _mm_setr_ps(0.f, 0.f, 1.f, t.z));
  _mm_storeu_ps(&m.rows[3].x, _mm_setr_ps(0.f, 0.f, 0.f, 1.f));
  return m;
}

m4x4 
to_m4x4(Quaternion q) 
{
  m4x4 result = identity();
  f32 w = q.w;
  f32 x = q.x;
  f32 y = q.y;
  f32 z = q.z;

  result.e[0][0] = 1.0f - 2.0f * (y * y + z * z);
  result.e[0][1] = 2.0f * (x * y - z * w);
  result.e[0][2] = 2.0f * (x * z + y * w);

  result.e[1][0] = 2.0f * (x * y + z * w);
  result.e[1][1] = 1.0f - 2.0f * (x * x + z * z);
  result.e[1][2] = 2.0f * (y * z - x * w);

  result.e[2][0] = 2.0f * (x * z - y * w);
  result.e[2][1] = 2.0f * (y * z + x * w);
  result.e[2][2] = 1.0f - 2.0f * (x * x + y * y);

  return result;
}

// @Fix: I think it's wrong
Quaternion 
euler_to_quaternion(f32 roll, f32 pitch, f32 yaw) 
{
  f32 cr = m_cos(roll * 0.5f);
  f32 sr = m_sin(roll * 0.5f);
  f32 cp = m_cos(pitch * 0.5f);
  f32 sp = m_sin(pitch * 0.5f);
  f32 cy = m_cos(yaw * 0.5f);
  f32 sy = m_sin(yaw * 0.5f);

  Quaternion q;
  q.w = cr * cp * cy + sr * sp * sy;
  q.x = sr * cp * cy - cr * sp * sy;
  q.y = cr * sp * cy + sr * cp * sy;
  q.z = cr * cp * sy - sr * sp * cy;

  return q;
}

m4x4 
m4x4_scale(f32 x, f32 y, f32 z) 
{
  // SSE
  m4x4 m;
  _mm_storeu_ps(m.rows[0].e, _mm_setr_ps(x, 0, 0, 0));
  _mm_storeu_ps(m.rows[1].e, _mm_setr_ps(0, y, 0, 0));
  _mm_storeu_ps(m.rows[2].e, _mm_setr_ps(0, 0, z, 0));
  _mm_storeu_ps(m.rows[3].e, _mm_setr_ps(0, 0, 0, 1));
  return m;
}

Quaternion
build_quaternion(vec3 axis, f32 radian)
{
  f32 c = m_cos(radian*0.5f);
  f32 s = m_sin(radian*0.5f);
  vec3 n = s * normalize(axis);
  Quaternion result = make_quaternion(c, n.x, n.y, n.z);
  return result;
}

Quaternion 
rotate(Quaternion q0, vec3 axis, f32 radian)
{
  Quaternion result = build_quaternion(axis, radian) * q0;
  return result;
}

vec3
project(vec3 p, m4x4 view_proj)
{
  vec4 res = view_proj * vec4{p.x, p.y, p.z, 1};
  res.x /= res.w;
  res.y /= res.w;
  res.z /= res.w;
  return res.xyz;
}

f32 
radian_from_degree(f32 d) 
{
  return d * 0.01745329251f; // d * PI / 180
}

//
// Graphics
//
m4x4 
look_to_lh(vec3 from, vec3 to, vec3 up) 
{
  R_ASSERT(!is_zero(to));
  R_ASSERT(!is_inf(to));
  R_ASSERT(!is_zero(up));
  R_ASSERT(!is_inf(up));

  vec3 Z = normalize(to);
  vec3 X = normalize(cross(up, Z));
  vec3 Y = normalize(cross(Z, X));

  vec3 T = from;

  m4x4 M;

  M._11 = X.x;
  M._12 = X.y;
  M._13 = X.z;
  M._14 = -dot(T, X);

  M._21 = Y.x;
  M._22 = Y.y;
  M._23 = Y.z;
  M._24 = -dot(T, Y);

  M._31 = Z.x;
  M._32 = Z.y;
  M._33 = Z.z;
  M._34 = -dot(T, Z);

  M._41 = 0.f;
  M._42 = 0.f;
  M._43 = 0.f;
  M._44 = 1.f;

  return M;
}

m4x4 
look_to_rh(vec3 from, vec3 to, vec3 up) 
{
  return look_to_lh(from, -to, up);
}

m4x4 
look_at_lh(vec3 from, vec3 at, vec3 up) 
{
  vec3 to = at - from;
  m4x4 m = look_to_lh(from, to, up);
  return m;
}

m4x4 
look_at_rh(vec3 from, vec3 at, vec3 up) 
{
  vec3 neg_to = from - at;
  m4x4 m = look_to_lh(from, neg_to, up);
  return m;
}

m4x4 
persp_fov_lh(f32 fov, f32 aspect_ratio, f32 near_z, f32 far_z) 
{
  f32 h = m_tan(0.5f * fov);
  f32 w = h / aspect_ratio;

  m4x4 M;

  M._11 = w;
  M._12 = 0.f;
  M._13 = 0.f;
  M._14 = 0.f;

  M._21 = 0.f;
  M._22 = h;
  M._23 = 0.f;
  M._24 = 0.f;

  M._31 = 0.f;
  M._32 = 0.f;
  M._33 = far_z / (far_z - near_z);
  M._34 = (near_z * far_z) / (near_z - far_z);

  M._41 = 0.f;
  M._42 = 0.f;
  M._43 = 1.f;
  M._44 = 0.f;

  return M;
}

m4x4 
persp_fov_rh(f32 fov, f32 aspect_ratio, f32 near_z, f32 far_z) 
{
  f32 h = 1.f / m_tan(0.5f * fov);
  f32 w = h / aspect_ratio;

  m4x4 M;

  M._11 = w;
  M._12 = 0.f;
  M._13 = 0.f;
  M._14 = 0.f;

  M._21 = 0.f;
  M._22 = h;
  M._23 = 0.f;
  M._24 = 0.f;

  M._31 = 0.f;
  M._32 = 0.f;
  M._33 = far_z / (near_z - far_z);
  M._34 = (near_z * far_z) / (near_z - far_z);

  M._41 = 0.f;
  M._42 = 0.f;
  M._43 = -1.f;
  M._44 = 0.f;

  return M;
}

u32 
pack_rgba(vec4 rgba) 
{
  // SSE2
  __m128 f = _mm_loadu_ps(&rgba.x);
  f = _mm_mul_ps(f, _mm_set1_ps(255.f));
  __m128i i = _mm_cvtps_epi32(f);
  i = _mm_packs_epi32(i, i);
  i = _mm_packus_epi16(i, i);
  return (u32)_mm_cvtsi128_si32(i);
}

vec4 
unpack_rgba(u32 rgba) 
{
  // SSE2
  __m128i zero = _mm_setzero_si128();
  __m128i u = _mm_cvtsi32_si128((int)rgba);
  u = _mm_unpacklo_epi8(u, zero);
  u = _mm_unpacklo_epi16(u, zero);
  vec4 v;
  _mm_storeu_ps(&v.x, _mm_mul_ps(_mm_cvtepi32_ps(u), _mm_set1_ps(1.f / 255.f)));
  return v;
}

Rect
rect_union(Rect a, Rect b)
{
  // SSE
  Rect R = {};
  __m128 p = _mm_loadu_ps(&a.min.x);
  __m128 q = _mm_loadu_ps(&b.min.x);
  __m128 r = _mm_min_ps(p, q);
  __m128 s = _mm_max_ps(p, q);
  __m128 t = _mm_shuffle_ps(r, s, _MM_SHUFFLE(3, 2, 1, 0));
  _mm_storeu_ps(&R.min.x, t);
  return R;
}

vec2
rect_center(Rect r)
{
  return 0.5f * vec2((r.min.x + r.max.x), 
                     (r.min.y + r.max.y));
}

//
// AABBV
//
AABBV
aabbv_make(vec3 lo, vec3 hi)
{
  AABBV box;
  box.lo = _mm_setr_ps(lo.x, lo.y, lo.z, 0.f);
  box.hi = _mm_setr_ps(hi.x, hi.y, hi.z, 0.f);
  return box;
}

AABBV
aabbv_make(f32 lo_x, f32 lo_y, f32 lo_z, f32 hi_x, f32 hi_y, f32 hi_z)
{
  AABBV box;
  box.lo = _mm_setr_ps(lo_x, lo_y, lo_z, 0.f);
  box.hi = _mm_setr_ps(hi_x, hi_y, hi_z, 0.f);
  return box;
}

AABBV
aabbv_from_aabb(AABB box)
{
  AABBV b;
  b.lo = _mm_setr_ps(box.min.x, box.min.y, box.min.z, 0.f);
  b.hi = _mm_setr_ps(box.max.x, box.max.y, box.max.z, 0.f);
  return b;
}

AABB
aabb_from_aabbv(AABBV box)
{
  struct {
    AABB b;
    f32 _;
  } s;
  _mm_storeu_ps(&s.b.min.x, box.lo);
  _mm_storeu_ps(&s.b.max.x, box.hi);
  return s.b;
}

AABBV
aabbv_union(AABBV a, AABBV b)
{
  AABBV c;
  c.lo = _mm_min_ps(a.lo, b.lo);
  c.hi = _mm_max_ps(a.hi, b.hi);
  return c;
}

void
aabbv_expand(AABBV *box, f32 extent)
{
  __m128 extentv = _mm_setr_ps(extent, extent, extent, 0.f);
  box->lo = _mm_sub_ps(box->lo, extentv);
  box->hi = _mm_add_ps(box->hi, extentv);
}

AABBV
aabbv_translate(AABBV box, vec3 t)
{
  AABBV b;
  b.lo = _mm_add_ps(box.lo, _mm_setr_ps(t.x, t.y, t.z, 0.f));
  b.hi = _mm_add_ps(box.hi, _mm_setr_ps(t.x, t.y, t.z, 0.f));
  return b;
}

f32
aabbv_surface(AABBV box)
{
  __m128 a = _mm_sub_ps(box.hi, box.lo);
  __m128 b = _mm_shuffle_ps(a, a, _MM_SHUFFLE(3, 0, 2, 1));
  return 2.f * dotv(a, b);
}

b32 
aabbv_contains(AABBV out, AABBV in)
{
  __m128 mask1 = _mm_cmple_ps(out.lo, in.lo);
  __m128 mask2 = _mm_cmpge_ps(out.hi, in.hi);
  __m128 mask  = _mm_and_ps(mask1, mask2);
  return (_mm_movemask_ps(mask) & 0x7) == 0x7;
}
