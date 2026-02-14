// Copyright Seong Woo Lee. All Rights Reserved.


f32 map(f32 x, f32 min, f32 max) {
    f32 t;
    f32 range = max - min;
    if (range != 0.0f) {
        t = (x - min) / range;
    } else {
        t = 0.0f;
    }
    return t;
}

f32 map01(f32 x, f32 min, f32 max) {
    return clamp01(map(x, min, max));
}

f32 map01_binormal(f32 x, f32 min, f32 max) {
    return 2.0f * map01(x, min, max) - 1.0f;
}

f32 binormal_to_normal(f32 x) {
    return x * 0.5f + 0.5f;
}

f32 lerp(f32 a, f32 t, f32 b) {
    return a + (b - a) * t;
}

f32 hermite(f32 a, f32 t, f32 b) {
    f32 t2 = t * t;
    f32 t3 = t2 * t;
    return lerp(a, t3 * (6 * t2 - 15 * t + 10), b);
}

f32 smoothstep(f32 x, f32 min, f32 max) {
    f32 p = map01(x, min, max);
    f32 v = p*p*(3.f - 2.f*p);
    return v;
}

f32 safe_ratio(f32 a, f32 b) {
    if (b != 0) return a / b;
    return 0.0f;
}

v2::v2(f32 f) {
    e[0] = f;
    e[1] = f;
}

v2::v2(f32 x_, f32 y_) {
    e[0] = x_;
    e[1] = y_;
}

v2 operator-(const v2 &in) {
    v2 V;
    V.x = -in.x;
    V.y = -in.y;
    return V;
}

v2 operator*(f32 f, v2 v) {
    v.x *= f;
    v.y *= f;
    return v;
}

v2 operator*(v2 v, f32 f) {
    v.x *= f;
    v.y *= f;
    return v;
}

v2 operator+(v2 a, v2 b) {
    v2 v;
    v.x = a.x + b.x;
    v.y = a.y + b.y;
    return v;
}

v2 operator-(v2 a, v2 b) {
    v2 v;
    v.x = a.x - b.x;
    v.y = a.y - b.y;
    return v;
}

internal v2&
operator+=(v2& a, v2 b) 
{
    a.x += b.x;
    a.y += b.y;

    return a;
}

internal v2&
operator-=(v2& a, v2 b) 
{
    a.x -= b.x;
    a.y -= b.y;

    return a;
}

internal v2&
operator*=(v2& a, f32 b) 
{
    a.x *= b;
    a.y *= b;

    return a;
}

internal v2
binormal_to_normal(v2 x)
{
    return (0.5f*x) + v2{0.5f, 0.5f};
}

internal f32 triarea2(v2 a, v2 b, v2 c) {
    v2 p = c - b;
    v2 q = a - b;
    return p.x*q.y - p.y*q.x;
}

internal f64 fmod_cycling(f64 x, f64 y) {
    assert( y != 0 );
    f64 remainder = x - (floor(x/y) * y);
    return remainder;
}

internal f32 fmod_cycling(f32 x, f32 y) {
    return (f32)fmod_cycling((f64)x, (f64)y);
}

internal f32
sqlen(v2 v) 
{
    return dot(v,v);
}

internal f32
sqlen(v3 v) 
{
    return dot(v,v);
}

internal f32
invsqlen(v2 v) 
{
    f32 result = 1.f / dot(v, v);
    return result;
}

internal f32
invsqlen(v3 v) 
{
    f32 result = 1.f / dot(v, v);
    return result;
}

internal f32
invsqlen(v4 v) 
{
    f32 result = 1.f / dot(v, v);
    return result;
}

internal f32
length(v2 A) 
{
    f32 result = sqrtf(sqlen(A));
    return result;
}

v2 lerp(v2 a, f32 t, v2 b) {
    v2 result = {};
    result.x = lerp(a.x, t, b.x);
    result.y = lerp(a.y, t, b.y);
    return result;
}

v2 normalize(v2 v) {
    f32 d = (v.x*v.x + v.y*v.y);
    assert(d != 0.f);
    v.x /= d;
    v.y /= d;
    return v;
}

v3::v3(f32 f) {
    e[0] = f;
    e[1] = f;
    e[2] = f;
}

v3::v3(f32 x_, f32 y_, f32 z_) {
    e[0] = x_;
    e[1] = y_;
    e[2] = z_;
}

internal v3
V3(f32 x, f32 y, f32 z)
{
    v3 v = {};
    v.x = x;
    v.y = y;
    v.z = z;
    return v;
}

internal v3
V3(v2 xy, f32 z)
{
    v3 v = {};
    v.x = xy.x;
    v.y = xy.y;
    v.z = z;
    return v;
}

internal v3
V3(f32 a)
{
    return v3{a,a,a};
}

internal b32
operator == (v3 a, v3 b)
{
    if (a.x == b.x && a.y == b.y && a.z == b.z) {
        return true;
    }
    return false;
}

internal v3
operator - (const v3 &in) 
{
    v3 V;
    V.x = -in.x;
    V.y = -in.y;
    V.z = -in.z;
    return V;
}

internal v3
operator * (f32 A, v3 B) 
{
    v3 result;
    result.x = A * B.x;
    result.y = A * B.y;
    result.z = A * B.z;

    return result;
}

internal v3
operator * (v3 B, f32 A) 
{
    v3 result;
    result.x = A * B.x;
    result.y = A * B.y;
    result.z = A * B.z;

    return result;
}

internal v3
operator / (v3 a, f32 b) 
{
    v3 result;
    result.x = a.x/b;
    result.y = a.y/b;
    result.z = a.z/b;
    return result;
}

internal v3&
operator /= (v3& a, f32 b) 
{
    f32 c = (1.0f / b);
    a.x *= c;
    a.y *= c;
    a.z *= c;
    return a;
}

internal v3
operator + (v3 A, v3 B) 
{
    v3 result;
    result.x = A.x + B.x;
    result.y = A.y + B.y;
    result.z = A.z + B.z;

    return result;
}

internal v3
operator - (v3 A, v3 B) 
{
    v3 result;
    result.x = A.x - B.x;
    result.y = A.y - B.y;
    result.z = A.z - B.z;

    return result;
}

internal v3&
operator += (v3& a, v3 b) 
{
    a.x += b.x;
    a.y += b.y;
    a.z += b.z;

    return a;
}

internal v3&
operator -= (v3& a, v3 b) 
{
    a.x -= b.x;
    a.y -= b.y;
    a.z -= b.z;

    return a;
}

internal v3&
operator *= (v3& a, f32 b) 
{
    a.x *= b;
    a.y *= b;
    a.z *= b;

    return a;
}

internal f32
dot(v2 a, v2 b) 
{
    f32 result = a.x*b.x + a.y*b.y;
    return result;
}

internal f32
dot(v3 a, v3 b) 
{
    f32 result = (a.x*b.x + a.y*b.y + a.z*b.z);
    return result;
}

#if SSE_ENABLED
f32 dot(__m128 a, __m128 b) {
    f32 result;
    __m128 p = _mm_mul_ps(a, b);
    __m128 q = _mm_shuffle_ps(p, p, _MM_SHUFFLE(2, 3, 0, 1));
    p = _mm_add_ps(p, q);
    q = _mm_shuffle_ps(p, p, _MM_SHUFFLE(0, 1, 2, 3));
    p = _mm_add_ps(p, q);
    _mm_store_ss(&result, p);
    return result;
}
#endif

f32 dot(v4 a, v4 b) {
#if SSE_ENABLED
    return dot(a.sse, b.sse);
#else
    f32 result = (a.r * b.r) + (a.g * b.g) + (a.b * b.b) + (a.a * b.a);
    return result;
#endif
}

v3 cross(v3 a, v3 b) {
    v3 v;
    v.x = (a.y*b.z) - (b.y*a.z);
    v.y = (a.z*b.x) - (b.z*a.x);
    v.z = (a.x*b.y) - (b.x*a.y);
    return v;
}

internal v2
hadamard(v2 a, v2 b) 
{
    v2 v = { a.x*b.x, a.y*b.y };
    return v;
}

internal v3
hadamard(v3 a, v3 b) 
{
    v3 v = { a.x*b.x, a.y*b.y, a.z*b.z };
    return v;
}

v4 hadamard(v4 a, v4 b) {
    v4 v;
#if SSE_ENABLED
    __m128 w = _mm_mul_ps(a.sse, b.sse);
    v.sse = w;
#else
    v = { a.x*b.x, a.y*b.y, a.z*b.z, a.w*b.w };
#endif
    return v;
}

internal f32
length(v3 A) 
{
    f32 result = sqrtf(sqlen(A));
    return result;
}

internal v3
normalize(v3 a) 
{
    v3 r = a;
    f32 inv_len = (1.0f / length(r));
    r *= inv_len;
    return r;
}

internal v3
lerp(v3 a, f32 t, v3 b)
{
    v3 result = {};
    result.x = lerp(a.x, t, b.x);
    result.y = lerp(a.y, t, b.y);
    result.z = lerp(a.z, t, b.z);
    return result;
}

f32 distance(v3 a, v3 b) {
    f32 dx = a.x - b.x;
    f32 dy = a.y - b.y;
    f32 dz = a.z - b.z;
    return sqrtf(dx*dx + dy*dy + dz*dz);
}

f32 distance(v2 a, v2 b) {
    f32 dx = a.x - b.x;
    f32 dy = a.y - b.y;
    return sqrtf(dx*dx + dy*dy);
}

internal f32
point_line_distance(v2 p, v2 a, v2 b)
{
    v2 u = p-a;
    v2 v = b-a;
    f32 div = 1.f / length(v);
    f32 result = absolute(u.x*v.y - u.y*v.x) * div;
    return result;
}

//
// Vector4 
//
internal v4
V4(f32 x) 
{
    return v4{x,x,x,x};
}

internal v4
V4(f32 r, f32 g, f32 b, f32 a) 
{
    v4 v = {};
    v.r = r;
    v.g = g;
    v.b = b;
    v.a = a;
    return v;
}

internal v4
V4(v2 rg, f32 b, f32 a)
{
    v4 v = {};
    v.r = rg.x;
    v.g = rg.y;
    v.b = b;
    v.a = a;
    return v;
}

internal v4
V4(v3 rgb, f32 a) 
{
    v4 v = {};
    v.rgb = rgb;
    v.a = a;
    return v;
}

v4 operator + (v4 a, v4 b) {
#if SSE_ENABLED
    v4 v;
    v.sse = _mm_add_ps(a.sse, b.sse);
    return v;
#else
    return v4{a.x+b.x, a.y+b.y, a.z+b.z, a.w+b.w};
#endif
}

v4 operator * (v4 v, f32 f) {
#if SSE_ENABLED
    __m128 f_ = _mm_set1_ps(f);
    v.sse = _mm_mul_ps(f_, v.sse);
    return v;
#else
    return v4{v.r*f, v.g*f, v.b*f, v.a*f};
#endif
}

v4 operator * (f32 f, v4 v) {
    return v * f;
}

v4 lerp(v4 a, f32 t, v4 b) {
#if SSE_ENABLED
    __m128 ax4 = a.sse;
    __m128 bx4 = b.sse;
    __m128 tx4 = _mm_set1_ps(t);
    __m128 dx4 = _mm_sub_ps(bx4, ax4);
    v4 v;
    v.sse = _mm_add_ps(ax4, _mm_mul_ps(tx4, dx4));
    return v;
#else
    v4 v;
    v.r = lerp(a.r, t, b.r);
    v.g = lerp(a.g, t, b.g);
    v.b = lerp(a.b, t, b.b);
    v.a = lerp(a.a, t, b.a);
    return v; 
#endif
}


//
// Quaternion
//
Quaternion::Quaternion() {
#if SSE_ENABLED
    sse = _mm_setr_ps(1.f, 0.f, 0.f, 0.f);
#else
    w = 1.f;
    x = 0.f;
    y = 0.f;
    z = 0.f;
#endif
}

Quaternion::Quaternion(f32 w_, f32 x_, f32 y_, f32 z_) {
#if SSE_ENABLED
    sse = _mm_setr_ps(w_, x_, y_, z_);
#else
    w = w_;
    x = x_;
    y = y_;
    z = z_;
#endif
}

Quaternion operator + (Quaternion a, Quaternion b) {
#if SSE_ENABLED
    Quaternion quat = {};
    quat.sse = _mm_add_ps(a.sse, b.sse);
    return quat;
#else
    Quaternion quat = {};
    quat.w = a.w + b.w;
    quat.x = a.x + b.x;
    quat.y = a.y + b.y;
    quat.z = a.z + b.z;
    return quat;
#endif
}

Quaternion operator - (Quaternion l, Quaternion r) {
#if SSE_ENABLED
    Quaternion quat = {};
    quat.sse = _mm_sub_ps(l.sse, r.sse);
    return quat;
#else
    Quaternion quat = {};
    quat.w = l.w - r.w;
    quat.x = l.x - r.x;
    quat.y = l.y - r.y;
    quat.z = l.z - r.z;
    return quat;
#endif
}

Quaternion
operator * (Quaternion a, Quaternion b)
{
    Quaternion q = {};
    {
        q.w = (a.w * b.w) - (a.x * b.x) - (a.y * b.y) - (a.z * b.z); 
        q.x = (a.w * b.x) + (a.x * b.w) + (a.y * b.z) - (a.z * b.y); 
        q.y = (a.w * b.y) + (a.y * b.w) + (a.z * b.x) - (a.x * b.z); 
        q.z = (a.w * b.z) + (a.z * b.w) + (a.x * b.y) - (a.y * b.x); 
    }

    return q;
}

Quaternion operator * (Quaternion q, f32 f) {
#if SSE_ENABLED
    __m128 q_ = q.sse;
    __m128 f_ = _mm_set1_ps(f);
    __m128 qf = _mm_mul_ps(q_, f_);
    Quaternion quat;
    quat.sse = qf;
    return quat;
#else
    Quaternion quat = {};
    quat.w = q.w * f;
    quat.x = q.x * f;
    quat.y = q.y * f;
    quat.z = q.z * f;
    return quat;
#endif
}

Quaternion operator * (f32 f, Quaternion q) {
#if SSE_ENABLED
    __m128 q_ = q.sse;
    __m128 f_ = _mm_set1_ps(f);
    __m128 qf = _mm_mul_ps(q_, f_);
    Quaternion quat;
    quat.sse = qf;
    return quat;
#else
    Quaternion quat = {};
    quat.w = q.w * f;
    quat.x = q.x * f;
    quat.y = q.y * f;
    quat.z = q.z * f;
    return quat;
#endif
}

Quaternion operator - (Quaternion q) {
#if SSE_ENABLED
    __m128 f = _mm_set1_ps(-1.f);
    q.sse = _mm_mul_ps(q.sse, f);
    return q;
#else
    q.w *= -1.f;
    q.x *= -1.f;
    q.y *= -1.f;
    q.z *= -1.f;
    return q;
#endif
}

Quaternion normalize(Quaternion q) {
    f32 d = 1.f / sqrtf(q.w*q.w + q.x*q.x + q.y*q.y + q.z*q.z);
#if SSE_ENABLED
    __m128 d_ = _mm_set1_ps(d);
    q.sse = _mm_mul_ps(q.sse, d_);
    return q;
#else
    q.w *= d;
    q.x *= d;
    q.y *= d;
    q.z *= d;
    return q;
#endif
}


f32 dot(Quaternion a, Quaternion b) {
#if SSE_ENABLED
    return dot(a.sse, b.sse);
#else
    f32 result = (a.w * b.w) + (a.x * b.x) + (a.y * b.y) + (a.z * b.z);
    return result;
#endif
}

Quaternion nlerp(Quaternion a, f32 t, Quaternion b) {
#if SSE_ENABLED
    __m128 ax4 = a.sse;
    __m128 bx4 = b.sse;
    __m128 tx4 = _mm_set1_ps(t);
    __m128 dx4 = _mm_sub_ps(bx4, ax4);
    Quaternion result;
    result.sse = _mm_add_ps(ax4, _mm_mul_ps(tx4, dx4));
    return result;
#else
    Quaternion quat;
    quat.w = lerp(a.w, t, b.w);
    quat.x = lerp(a.x, t, b.x);
    quat.y = lerp(a.y, t, b.y);
    quat.z = lerp(a.z, t, b.z);
    return quat;
#endif
}

Quaternion slerp(Quaternion q1, f32 t, Quaternion q2) {
    Quaternion result;

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
    f32 threshold = epsilon_f32;

    if (1.0f - cosom > threshold) {
        f32 omega, sinom;
        omega = acosf(cosom);
        sinom = sinf(omega);
        sclp  = sinf((1.0f - t) * omega) / sinom;
        sclq  = sinf(t * omega) / sinom;
    } else {
        sclp = 1.0f - t;
        sclq = t;
    }

    result.x = sclp * q1.x + sclq * q3.x;
    result.y = sclp * q1.y + sclq * q3.y;
    result.z = sclp * q1.z + sclq * q3.z;
    result.w = sclp * q1.w + sclq * q3.w;

    return result;
}

//
// m4x4
//
m4x4 operator * (m4x4 a, m4x4 b) {
#if SSE_ENABLED
    auto linear_combine = [](__m128 a, m4x4 b) {
        __m128 result;
        result = _mm_mul_ps(_mm_shuffle_ps(a, a, 0x00), b.rows[0].sse);
        result = _mm_add_ps(result, _mm_mul_ps(_mm_shuffle_ps(a, a, 0x55), b.rows[1].sse));
        result = _mm_add_ps(result, _mm_mul_ps(_mm_shuffle_ps(a, a, 0xaa), b.rows[2].sse));
        result = _mm_add_ps(result, _mm_mul_ps(_mm_shuffle_ps(a, a, 0xff), b.rows[3].sse));
        return result;
    };

    m4x4 result;
    __m128 out1x = linear_combine(a.rows[0].sse, b);
    __m128 out2x = linear_combine(a.rows[1].sse, b);
    __m128 out3x = linear_combine(a.rows[2].sse, b);
    __m128 out4x = linear_combine(a.rows[3].sse, b);

    result.rows[0].sse = out1x;
    result.rows[1].sse = out2x;
    result.rows[2].sse = out3x;
    result.rows[3].sse = out4x;

    return result;
#else
    m4x4 m = {};
    for (int r = 0; r < 4; ++r) {
        for (int c = 0; c < 4; ++c) {
            for (int i = 0; i < 4; ++i) {
                m.e[r][c] += a.e[r][i] * b.e[i][c];
            }
        }
    }
    return m;
#endif
}

m4x4& operator *= (m4x4& m, f32 f) {
    m = m * f;
    return m;
}

m4x4 operator * (m4x4 m, f32 f) {
    m.rows[0] = f * m.rows[0];
    m.rows[1] = f * m.rows[1];
    m.rows[2] = f * m.rows[2];
    m.rows[3] = f * m.rows[3];
    return m;
}

m4x4 operator * (f32 f, m4x4 m) {
    return m * f;
}

v4 operator * (m4x4 m, v4 p) {
    v4 res = v4{};
    for (int i = 0 ; i < 4; ++i) {
        for (int j = 0 ; j < 4; ++j) { 
            res.e[i] += (m.e[i][j] * p.e[j]);
        }
    }
    return res;
}

m4x4& operator += (m4x4& l, m4x4 r) {
    l.rows[0] = l.rows[0] + r.rows[0];
    l.rows[1] = l.rows[1] + r.rows[1];
    l.rows[2] = l.rows[2] + r.rows[2];
    l.rows[3] = l.rows[3] + r.rows[3];
    return l;
}

m4x4 identity() {
#if SSE_ENABLED
    m4x4 m;
    m.rows[0].sse = _mm_setr_ps(1.f, 0.f, 0.f, 0.f);
    m.rows[1].sse = _mm_setr_ps(0.f, 1.f, 0.f, 0.f);
    m.rows[2].sse = _mm_setr_ps(0.f, 0.f, 1.f, 0.f);
    m.rows[3].sse = _mm_setr_ps(0.f, 0.f, 0.f, 1.f);
    return m;
#else
    m4x4 m;
    m = {
        1.f, 0.f, 0.f, 0.f,
        0.f, 1.f, 0.f, 0.f,
        0.f, 0.f, 1.f, 0.f,
        0.f, 0.f, 0.f, 1.f
    };
    return m;
#endif
}

m4x4 x_rotation(f32 a) {
    f32 c = cosf(a);
    f32 s = sinf(a);
    m4x4 r = {
        1,  0,  0,  0,
        0,  c, -s,  0,
        0,  s,  c,  0,
        0,  0,  0,  1
    };

    return r;
}

m4x4 y_rotation(f32 a) {
    f32 c = cosf(a);
    f32 s = sinf(a);
    m4x4 r = {
        c,  0,  s,  0,
        0,  1,  0,  0,
       -s,  0,  c,  0,
        0,  0,  0,  1
    };

    return r;
}

m4x4 z_rotation(f32 a) {
    f32 c = cosf(a);
    f32 s = sinf(a);
    m4x4 r = {
        c, -s,  0,  0,
        s,  c,  0,  0,
        0,  0,  1,  0,
        0,  0,  0,  1
    };

    return r;
}

m4x4 transpose(m4x4 m) {
    m._12 = m._21;
    m._13 = m._31;
    m._14 = m._41;

    m._21 = m._12;
    m._23 = m._32;
    m._24 = m._42;

    m._31 = m._13;
    m._32 = m._23;
    m._34 = m._43;

    m._41 = m._14;
    m._42 = m._24;
    m._43 = m._34;

    return m;
}

m4x4 inverse(m4x4 m) {
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

m4x4 rows(v3 x, v3 y, v3 z) {
    m4x4 r = {
        x.x, x.y, x.z, 0,
        y.x, y.y, y.z, 0,
        z.x, z.y, z.z, 0,
        0, 0, 0, 1
    };
    return r;
}

m4x4 columns(v3 x, v3 y, v3 z) {
    m4x4 r = {
        x.x, y.x, z.x, 0,
        x.y, y.y, z.y, 0,
        x.z, y.z, z.z, 0,
        0, 0, 0, 1
    };
    return r;
}

m4x4 translate(m4x4 m, v3 t) {
    m._14 = t.x;
    m._24 = t.y;
    m._34 = t.z;
    return m;
}

m4x4 quaternion_to_m4x4(Quaternion q) {
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

Quaternion euler_to_quaternion(f32 roll, f32 pitch, f32 yaw) {
    f32 cr = cosf(roll * 0.5f);
    f32 sr = sinf(roll * 0.5f);
    f32 cp = cosf(pitch * 0.5f);
    f32 sp = sinf(pitch * 0.5f);
    f32 cy = cosf(yaw * 0.5f);
    f32 sy = sinf(yaw * 0.5f);

    Quaternion q;
    q.w = cr * cp * cy + sr * sp * sy;
    q.x = sr * cp * cy - cr * sp * sy;
    q.y = cr * sp * cy + sr * cp * sy;
    q.z = cr * cp * sy - sr * sp * cy;

    return q;
}

internal m4x4
scale(m4x4 transform, v3 factor) 
{
    m4x4 result = transform;
    for (int i = 0; i < 3; ++i) {
        result.e[i][i] *= factor.e[i];
    }
    return result;
}

internal m4x4
scale(m4x4 transform, f32 factor) 
{
    m4x4 result = transform;
    for (int i = 0; i < 3; ++i) {
        result.e[i][i] *= factor;
    }
    return result;
}

m4x4 scale(f32 f) {
    m4x4 m = {};
    m._11 = f;
    m._22 = f;
    m._33 = f;
    m._44 = 1.f;
    return m;
}

internal m4x4
camera_transform(v3 x, v3 y, v3 z, v3 p) 
{
    m4x4 R = rows(x, y, z);
    R = translate(R, -(R * V4(p, 0)).xyz);

    return R;
}

internal v3
get_row(m4x4 M, u32 R) 
{
    v3 V = {
        M.e[R][0],
        M.e[R][1],
        M.e[R][2]
    };
    return V;
}

internal v3
get_column(m4x4 M, u32 C) 
{
    v3 V = {
        M.e[0][C],
        M.e[1][C],
        M.e[2][C]
    };
    return V;
}

//
// Rect
//
internal Rect2
rect2_min_max(v2 min, v2 max)
{
    Rect2 result = {};
    result.min = min;
    result.max = max;
    return result;
}

internal Rect2
rect2_cen_half_dim(v2 cen, v2 h_dim)
{
    Rect2 result = {};
    result.min = cen - h_dim;
    result.max = cen + h_dim;
    return result;
}

internal Rect2
rect2_min_dim(v2 min, v2 dim)
{
    Rect2 result = {};
    result.min = min;
    result.max = min + dim;
    return result;
}

internal Rect2
rect2_inv_inf()
{
    Rect2 result = {};
    result.min.x =  F32_MAX;
    result.min.y =  F32_MAX;
    result.max.x = -F32_MAX;
    result.max.y = -F32_MAX;
    return result;
}

internal Rect2
offset(Rect2 rect, v2 offset)
{
    Rect2 result = {};
    result.min = rect.min + offset;
    result.max = rect.max + offset;
    return result;
}

internal Rect2
add_radius_to(Rect2 rect, v2 radius)
{
    Rect2 result = rect;
    result.min -= radius;
    result.max += radius;
    return result;
}

m4x4 m4x4_from_trs(v3 translation, Quaternion rotation, v3 scaling) {
    m4x4 T = translate(identity(), translation);
    m4x4 R = quaternion_to_m4x4(rotation);
    m4x4 S = scale(identity(), scaling);
    m4x4 result = T * R * S;
    return result;
}

internal Quaternion
build_quaternion(v3 axis, f32 radian)
{
    f32 c = cosf(radian*0.5f);
    f32 s = sinf(radian*0.5f);
    v3 n = s * normalize(axis);
    Quaternion result = Quaternion{c, n.x, n.y, n.z};
    return result;
}

internal Quaternion 
rotate(Quaternion q0, v3 axis, f32 radian)
{
    Quaternion result = build_quaternion(axis, radian) * q0;
    return result;
}

internal v3
project(v3 p, m4x4 view_proj)
{
    v4 res = view_proj * v4{p.x, p.y, p.z, 1};
    res.x /= res.w;
    res.y /= res.w;
    res.z /= res.w;
    return res.xyz;
}

internal v2
V2(v2u v) 
{
    v2 result = v2{(f32)v.x, (f32)v.y};
    return result;
}

internal m4x4
lookat(v3 eye, v3 center, v3 up_) 
{
    v3 forward = normalize(center - eye);
    v3 side    = normalize(cross(normalize(up_), forward));
    v3 up      = normalize(cross(forward, side));

    m4x4 result = camera_transform(-side, up, -forward, eye);
    return result;
}

internal m4x4
view_transform(v3 position, Quaternion orientation)
{
    m4x4 rotation = quaternion_to_m4x4(orientation);
    m4x4 result = camera_transform(get_column(rotation, 0),
                                   get_column(rotation, 1),
                                   get_column(rotation, 2),
                                   position);
    return result;
}

// @Todo: Opengl's clip-space's z range is [-1,1] while d3d's is [0,1].
internal m4x4
ortho(f32 min_x, f32 max_x, f32 min_y, f32 max_y, f32 min_z, f32 max_z) 
{
    f32 a = safe_ratio(2.0f, max_x - min_x);
    f32 b = safe_ratio(min_x + max_x, min_x - max_x);
    f32 c = safe_ratio(2.0f, max_y - min_y);
    f32 d = safe_ratio(min_y + max_y, min_y - max_y);
    f32 N = min_z;
    f32 F = max_z;
    f32 e = safe_ratio(2.0f , (N - F));
    f32 f = safe_ratio((N + F), (N - F));
    m4x4 result = m4x4{{
        { a,  0,  0,  b},
        { 0,  c,  0,  d},
        { 0,  0,  e,  f},
        { 0,  0,  0,  1}
    }};
    return result;
}

internal f32
radian_from_degree(f32 d) 
{
    return d*pi32*0.005556f;
}

internal f32
normalize01(v2 range, f32 val)
{
    f32 result = 0.0f;
    f32 denom = (range.y - range.x);
    f32 numer = (val - range.x);
    if (denom != 0.0f)
    { result = clamp(numer/denom, 0.0f, 1.0f); }
    return result;
}

internal b32
intersects(AABB2 box, v2 point)
{
    b32 result = false;
    // TODO: Define boundary.
    if (point.x >= box.min.x && point.x < box.max.x &&
        point.y >= box.min.y && point.y < box.max.y) 
    { result = true; }
    return result;
}

internal b32
intersects(AABB2 a, AABB2 b)
{
    b32 result = false;
    v2 half_dim = (a.max - a.min) * 0.5f;
    v2 point = (a.min + a.max) * 0.5f;
    b.min -= half_dim;
    b.max += half_dim;
    result = intersects(b, point);
    return result;
}

internal AABB2
intersection(AABB2 a, AABB2 b)
{
    AABB2 result = {};
    if (intersects(a, b))
    {
        result.min.x = (a.min.x >= b.min.x) ? a.min.x : b.min.x; 
        result.max.x = (a.max.x >= b.max.x) ? b.max.x : a.max.x;
        result.min.y = (a.min.y >= b.min.y) ? a.min.y : b.min.y; 
        result.max.y = (a.max.y >= b.max.y) ? b.max.y : a.max.y;
    }
    return result;
}

internal AABB2
aabb2_infinite(void)
{
    AABB2 result = {};
    result.min = v2{-F32_MAX, -F32_MAX};
    result.max = v2{ F32_MAX,  F32_MAX};
    return result;
}



internal v2
to_ndc(v2 p, f32 w, f32 h)
{
    f32 x = 2.f*( p.x / w) - 1.f;
    f32 y = 2.f*(-p.y / h) + 1.f;
    v2 result = {x,y};
    return result;
}

internal v3
unproject(v3 position, m4x4 viewproj)
{
    m4x4 inv_viewproj = inverse(viewproj);
    v4 h = inv_viewproj*V4(position, 1.f);
    v3 result = h.xyz / h.w;
    return result;
}

internal Ray3
ray_from_screen_position(v2 position, f32 screen_width, f32 screen_height, m4x4 viewproj)
{
    Ray3 result = {};

    f32 x = 2.f*( position.x / screen_width ) - 1.f;
    f32 y = 2.f*(-position.y / screen_height) + 1.f;

    m4x4 inv_viewproj = inverse(viewproj);

    v4 near_clip = v4{x, y, NEAR_Z, 1.f};
    v4 far_clip  = v4{x, y,  FAR_Z, 1.f};

    v4 near_p = inv_viewproj*near_clip;
    v4 far_p  = inv_viewproj*far_clip;

    near_p.xyz = near_p.xyz / near_p.w;
    far_p.xyz  = far_p.xyz  / far_p.w;

    result.origin   = near_p.xyz;
    result.direction = normalize(far_p.xyz - near_p.xyz);

    return result;
}

bool ray_plane_intersect(Ray3 ray, v3 plane_normal, f32 plane_height, v3* out) {
    v3  n = plane_normal;
    f32 d = plane_height;

    v3 o = ray.origin;
    v3 v = ray.direction;

    f32 t = 0.f;
    f32 denom = dot(v, n);

    if (absolute(denom) > 0.0001f) {
        t = -(dot(o, n) + d) / denom;
        *out = o + t*v;

        return true;
    } else {
        return false;
    }
}
