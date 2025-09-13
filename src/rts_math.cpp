/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Seong Woo Lee $
   $Notice: (C) Copyright %s by Seong Woo Lee. All Rights Reserved. $
   ======================================================================== */


internal f32
square(f32 val) {
    f32 result = val * val;
    return result;
}

internal f32
absolute(f32 val) {
    f32 result = (val > 0) ? val : -val;
    return result;
}

internal f32
map(f32 x, f32 min, f32 max) {
    f32 t;
    f32 range = max - min;
    if (range != 0.0f) {
        t = ((x - min) / range);
    } else {
        t = 0.0f;
    }
    return t;
}

internal f32
map01(f32 x, f32 min, f32 max) {
    return clamp01(map(x, min, max));
}

internal f32
map01_binormal(f32 x, f32 min, f32 max) {
    return 2.0f * map01(x, min, max) - 1.0f;
}

internal f32
binormal_to_normal(f32 x) {
    return x * 0.5f + 0.5f;
}

internal f32
lerp(f32 a, f32 t, f32 b) {
    f32 result = b * t + (1 - t) * a;
    return result;
}

internal f32
smoothstep(f32 x, f32 min, f32 max) {
    f32 p = map01(x, min, max);
    f32 v = p * p * (3.0f - 2.0f * p);
    return v;
}

internal f32
safe_ratio(f32 a, f32 b) {
    if (b != 0) return a / b;
    return 0.0f;
}

internal v2
V2(f32 x, f32 y) {
    return v2{x, y};
}

internal v2
V2(f32 x) {
    return v2{x, x};
}

internal v2
operator-(const v2 &in) {
    v2 V;
    V.x = -in.x;
    V.y = -in.y;
    return V;
}

internal v2
operator*(f32 A, v2 B) {
    v2 result;
    result.x = A * B.x;
    result.y = A * B.y;

    return result;
}

internal v2
operator*(v2 B, f32 A) 
{
    v2 result;
    result.x = A * B.x;
    result.y = A * B.y;

    return result;
}

internal v2
operator+(v2 A, v2 B) 
{
    v2 result;
    result.x = A.x + B.x;
    result.y = A.y + B.y;

    return result;
}

internal v2
operator-(v2 A, v2 B) 
{
    v2 result;
    result.x = A.x - B.x;
    result.y = A.y - B.y;

    return result;
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

internal f32
dot(v2 a, v2 b) 
{
    f32 result = a.x * b.x + a.y * b.y;
    return result;
}

internal v2
hadamard(v2 A, v2 B) 
{
    v2 result = {
        A.x * B.x,
        A.y * B.y,
    };

    return result;
}

internal v2
hadamard(v2 A, v2u B) 
{
    v2 result = {
        A.x * (f32)B.x,
        A.y * (f32)B.y,
    };

    return result;
}

internal v2
binormal_to_normal(v2 x)
{
    return (0.5f*x) + v2{0.5f, 0.5f};
}

internal f32
length_square(v2 A) {
    return dot(A, A);
}

internal f32
inv_length_square(v2 A) {
    f32 result = 1.0f / dot(A, A);
    return result;
}

internal f32
length(v2 A) {
    f32 result = sqrt(length_square(A));
    return result;
}

internal v2
normalize(v2 a) 
{
    v2 r = a;
    f32 inv_len = (1.0f / length(r));
    r *= inv_len;
    return r;
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
dot(v3 a, v3 b) 
{
    f32 result = (a.x * b.x +
                  a.y * b.y +
                  a.z * b.z);

    return result;
}

internal v3
hadamard(v3 A, v3 B) 
{
    v3 result = {
        A.x * B.x,
        A.y * B.y,
        A.z * B.z 
    };
    return result;
}

internal v3
cross(v3 A, v3 B) 
{
    v3 R = {};
    R.x = (A.y * B.z) - (B.y * A.z);
    R.y = (A.z * B.x) - (B.z * A.x);
    R.z = (A.x * B.y) - (B.x * A.y);
    return R;
}

internal f32
length_square(v3 A) 
{
    f32 result = dot(A, A);
    return result;
}

internal f32
length(v3 A) 
{
    f32 result = sqrt(length_square(A));
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
noz(v3 a)
{
    v3 result = {};

    f32 lensq = length_square(a);
    if (lensq > square(0.0001f)) {
        result = (1.0f / sqrt(lensq)) * a;
    }

    return result;
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

internal f32
distance(v3 a, v3 b) 
{
    return sqrt((a.x - b.x) * (a.x - b.x) + 
                (a.y - b.y) * (a.y - b.y) + 
                (a.z - b.z) * (a.z - b.z));
}

internal f32
distance(v2 a, v2 b) 
{
    return sqrt((a.x - b.x) * (a.x - b.x) + 
                (a.y - b.y) * (a.y - b.y));
}

internal f32
point_line_distance(v2 p, v2 a, v2 b)
{
    v2 u = p-a;
    v2 v = b-a;
    f32 div = 1.0f / length(v);
    f32 result = absolute(u.x*v.y - u.y*v.x) * div;
    return result;
}

internal v3
linear_to_srgb(v3 c)
{
    return v3{(f32)pow(c.x, 2.2f), (f32)pow(c.y, 2.2f), (f32)pow(c.z, 2.2f)};
}

internal v3
srgb_to_linear(v3 c)
{
    f32 t = 0.454545f;
    return v3{(f32)pow(c.x, t), (f32)pow(c.y, t), (f32)pow(c.z, t)};
}


//
// v4
//

internal v4
V4(f32 x) {
    return v4{x,x,x,x};
}

internal v4
V4(f32 r, f32 g, f32 b, f32 a) {
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
V4(v3 rgb, f32 a) {
    v4 v = {};
    v.rgb = rgb;
    v.a = a;
    return v;
}

internal v4
operator * (v4 a, f32 b) {
    return v4{a.x*b, a.y*b, a.z*b, a.w*b};
}

internal v4
lerp(v4 a, f32 t, v4 b)
{
    v4 result = {};
    result.r = lerp(a.r, t, b.r);
    result.g = lerp(a.g, t, b.g);
    result.b = lerp(a.b, t, b.b);
    result.a = lerp(a.a, t, b.a);
    return result;
}


//
// quaternion
//
internal Quaternion
operator + (Quaternion a, Quaternion b)
{
    Quaternion q = {};
    q.w = a.w + b.w;
    q.x = a.x + b.x;
    q.y = a.y + b.y;
    q.z = a.z + b.z;

    return q;
}

internal Quaternion
operator * (Quaternion a, Quaternion b)
{
    Quaternion q = {};
    q.w = (a.w * b.w) - (a.x * b.x) - (a.y * b.y) - (a.z * b.z); 
    q.x = (a.w * b.x) + (a.x * b.w) + (a.y * b.z) - (a.z * b.y); 
    q.y = (a.w * b.y) + (a.y * b.w) + (a.z * b.x) - (a.x * b.z); 
    q.z = (a.w * b.z) + (a.z * b.w) + (a.x * b.y) - (a.y * b.x); 

    return q;
}

internal Quaternion
operator * (Quaternion a, f32 b)
{
    Quaternion q = {};
    q.w = a.w * b;
    q.x = a.x * b;
    q.y = a.y * b;
    q.z = a.z * b;

    return q;
}

internal Quaternion
operator * (f32 b, Quaternion a)
{
    Quaternion q = {};
    q.w = a.w * b;
    q.x = a.x * b;
    q.y = a.y * b;
    q.z = a.z * b;

    return q;
}

internal Quaternion
operator - (Quaternion in)
{
    Quaternion q = {};
    q.w = -in.w;
    q.x = -in.x;
    q.y = -in.y;
    q.z = -in.z;

    return q;
}

internal f32
dot(Quaternion a, Quaternion b)
{
    f32 result = ( (a.w * b.w) +
                   (a.x * b.x) +
                   (a.y * b.y) +
                   (a.z * b.y) );
    return result;
}

internal Quaternion
slerp(Quaternion q1, f32 t, Quaternion q2)
{
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
        omega = acos(cosom);
        sinom = sin(omega);
        sclp  = sin((1.0f - t) * omega) / sinom;
        sclq  = sin(t * omega) / sinom;
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

internal m4x4
operator * (m4x4 a, m4x4 b) 
{
    m4x4 R = {};

    for (int r = 0; r < 4; ++r) {
        for (int c = 0; c < 4; ++c) {
            for (int i = 0; i < 4; ++i) {
                R.e[r][c] += a.e[r][i] * b.e[i][c];
            }
        }
    }

    return R;
}

internal v4
operator * (m4x4 m, v4 p)
{
    v4 res = v4{};
    for (int i = 0 ; i < 4; ++i) {
        for (int j = 0 ; j < 4; ++j) { 
            res.e[i] += (m.e[i][j] * p.e[j]);
        }
    }
    return res;
}

internal m4x4
identity() 
{
    m4x4 r = {
        {{ 1,  0,  0,  0 },
            { 0,  1,  0,  0 },
            { 0,  0,  1,  0 },
            { 0,  0,  0,  1 }},
    };
    return r;
}

internal m4x4
x_rotation(f32 a) 
{
    f32 c = cos(a);
    f32 s = sin(a);
    m4x4 r = {
        {{ 1,  0,  0,  0 },
            { 0,  c, -s,  0 },
            { 0,  s,  c,  0 },
            { 0,  0,  0,  1 }},
    };

    return r;
}

internal m4x4
y_rotation(f32 a) 
{
    f32 c = cos(a);
    f32 s = sin(a);
    m4x4 r = {
        {{ c,  0,  s,  0 },
            { 0,  1,  0,  0 },
            {-s,  0,  c,  0 },
            { 0,  0,  0,  1 }},
    };

    return r;
}

internal m4x4
z_rotation(f32 a) 
{
    f32 c = cos(a);
    f32 s = sin(a);
    m4x4 r = {
        {{ c, -s,  0,  0 },
            { s,  c,  0,  0 },
            { 0,  0,  1,  0 },
            { 0,  0,  0,  1 }}
    };

    return r;
}

internal m4x4
transpose(m4x4 m) 
{
    m4x4 r = {};
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            r.e[j][i] = m.e[i][j];
        }
    }
    return r;
}

internal m4x4
inverse(m4x4 m) 
{
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
    det = 1.0f / det;

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

internal m4x4
rows(v3 x, v3 y, v3 z) 
{
    m4x4 r = {
        {{ x.x, x.y, x.z,  0 },
            { y.x, y.y, y.z,  0 },
            { z.x, z.y, z.z,  0 },
            {   0,   0,   0,  1 }}
    };
    return r;
}

internal m4x4
columns(v3 x, v3 y, v3 z) 
{
    m4x4 r = {
        {{ x.x, y.x, z.x,  0 },
            { x.y, y.y, z.y,  0 },
            { x.z, y.z, z.z,  0 },
            {   0,   0,   0,  1 }}
    };
    return r;
}

internal m4x4
translate(m4x4 m, v3 t) 
{
    m4x4 result = m;
    result.e[0][3] += t.x;
    result.e[1][3] += t.y;
    result.e[2][3] += t.z;

    return result;
}

internal m4x4
quaternion_to_m4x4(Quaternion q) 
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

internal Quaternion
euler_to_quaternion(f32 roll, f32 pitch, f32 yaw)
{
    f32 cr = cos(roll * 0.5f);
    f32 sr = sin(roll * 0.5f);
    f32 cp = cos(pitch * 0.5f);
    f32 sp = sin(pitch * 0.5f);
    f32 cy = cos(yaw * 0.5f);
    f32 sy = sin(yaw * 0.5f);

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

internal m4x4
scale(f32 s) 
{
    m4x4 result = m4x4{{
        s, 0, 0, 0,
            0, s, 0, 0,
            0, 0, s, 0,
            0, 0, 0, 1,
    }};
    return result;
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

internal b32
in_rect(Rect2 rect, v2 p)
{
    b32 result = (rect.min.x <= p.x && 
                  rect.min.y <= p.y && 
                  rect.max.x > p.x && 
                  rect.max.y > p.y);
    return result;
}

internal v2
get_dim(Rect2 rect)
{
    v2 result = {};
    result.x = (rect.max.x - rect.min.x);
    result.y = (rect.max.y - rect.min.y);
    return result;
}

internal m4x4
trs_to_transform(v3 translation, Quaternion rotation, v3 scaling)
{
    m4x4 T = translate(identity(), translation);
    m4x4 R = quaternion_to_m4x4(rotation);
    m4x4 S = scale(identity(), scaling);
    m4x4 result = T * R * S;
    return result;
}

internal Quaternion
build_quaternion(v3 axis, f32 radian)
{
    f32 c = cos(radian*0.5f);
    f32 s = sin(radian*0.5f);
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
lookat(v3 eye, v3 center, v3 up_) {
    v3 forward = normalize(center - eye);
    v3 side = normalize(cross(normalize(up_), forward));
    v3 up = normalize(cross(forward, side));

    m4x4 result = camera_transform(-side, up, -forward, eye);
    return result;
}

internal m4x4
view_transform(v3 position, Quaternion orientation) {
    m4x4 rotation = quaternion_to_m4x4(orientation);
    m4x4 result = camera_transform(get_column(rotation, 0),
                                   get_column(rotation, 1),
                                   get_column(rotation, 2),
                                   position);
    return result;
}

// @TODO: Opengl's clip-space's z range is [-1,1] while d3d's is [0,1].
internal m4x4
ortho(f32 min_x, f32 max_x, f32 min_y, f32 max_y, f32 min_z, f32 max_z) {
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
degrees_to_radian(f32 x) {
    return x*pi32*0.005556f;
}

internal b32
point_in_aabb(v3 p, AABB3 aabb) {
    if (p.x >= aabb.min.x && p.x < aabb.max.x &&
        p.y >= aabb.min.y && p.y < aabb.max.y &&
        p.z >= aabb.min.z && p.z < aabb.max.z) return true;
    return false;
}
