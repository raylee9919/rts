// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RTS_INT128_H
#define RTS_INT128_H

/*

  INT128_MIN and negation have some tricky edge cases that aren't handled, 
  but I don't think they're worth worrying about... at least for now!

                                                           -swL 2026-11-10
*/

#include "basic/core.h"

struct u128 {
  u64 lo;
  u64 hi;
};

struct s128 {
  u64 lo;
  s64 hi;
};


const read_only u128 U128_ERROR = { UINT64_MAX, UINT64_MAX };
const read_only s128 S128_ERROR = { UINT64_MAX,         -1 };


force_inline bool 
operator == (u128 a, u128 b)
{
  return (a.lo == b.lo) && (a.hi == b.hi);
}

force_inline bool 
operator == (s128 a, s128 b)
{
  return (a.lo == b.lo) && (a.hi == b.hi);
}

force_inline bool 
operator != (u128 a, u128 b)
{
  return (a.lo != b.lo) || (a.hi != b.hi);
}

force_inline bool 
operator != (s128 a, s128 b)
{
  return (a.lo != b.lo) || (a.hi != b.hi);
}

force_inline bool 
operator < (u128 a, u128 b)
{
#if COMPILER_CL 
  {
    u64 unused, borrow;
    borrow = _subborrow_u64(0, a.lo, b.lo, &unused);
    borrow = _subborrow_u64(borrow, a.hi, b.hi, &unused);
    return borrow != 0;
  }
#else 
  {
    // @Speed
    if (a.hi != b.hi) return (a.hi < b.hi);
    return a.lo < b.lo;
  }
#endif
}

force_inline bool 
operator > (u128 a, u128 b)
{
  return b < a;
}

force_inline bool 
operator >= (u128 a, u128 b)
{
  return !(a < b);
}

force_inline bool 
operator <= (u128 a, u128 b)
{
  return !(b < a);
}

force_inline u128
operator << (u128 a, int b)
{
  if (b == 0) return a;

  if (b >= 64) {
    return { 0, a.lo << (b - 64) };
  } else {
    return { a.lo << b, (a.hi << b) | (a.lo >> (64 - b)) };
  }
}

force_inline u128
operator >> (u128 a, int b)
{
  if (b == 0) return a;

  if (b >= 64) {
    return { a.hi >> (b - 64), 0 };
  } else {
    return { (a.lo >> b) | (a.hi << (64 - b)), a.hi >> b };
  }
}

force_inline u128&
operator <<= (u128& a, int b)
{
  return a = (a << b);
}

force_inline u128&
operator >>= (u128& a, int b)
{
  return a = (a >> b);
}

force_inline u128 
operator + (u128 a, u128 b)
{
#if COMPILER_CL 
  {
    u128 c;
    _addcarry_u64(_addcarry_u64(0, a.lo, b.lo, &c.lo), a.hi, b.hi, &c.hi);
    return c;
  }
#else 
  {
    // @Speed
    u128 c;
    c.lo = a.lo + b.lo;
    c.hi = a.hi + b.hi;
    if (c.lo < a.lo) c.hi += 1;
    return c;
  }
#endif
}

force_inline s128 
operator + (s128 a, s128 b)
{
  u128 c = u128{a.lo, (u64)a.hi} + u128{b.lo, (u64)b.hi};
  return s128{c.lo, (s64)c.hi};
}

force_inline u128 
operator - (u128 a, u128 b)
{
#if COMPILER_CL 
  {
    u128 c;
    _subborrow_u64(_subborrow_u64(0, a.lo, b.lo, &c.lo), a.hi, b.hi, &c.hi);
    return c;
  }
#else 
  {
    // @Speed
    u128 c;
    c.lo = a.lo - b.lo;
    c.hi = a.hi - b.hi;
    if (c.lo > a.lo) c.hi -= 1;
    return c;
  }
#endif
}

force_inline s128 
operator - (s128 a)
{
  // Two's complement negation.
  s128 b;
  b.lo = ~a.lo;
  b.hi = ~a.hi;
  b.lo += 1;
  if (b.lo == 0) b.hi += 1;
  return b;
}

force_inline u128&
operator += (u128& a, u128 b)
{
  return a = a + b;
}

force_inline u128&
operator -= (u128& a, u128 b)
{
  return a = a - b;
}

force_inline u128 
operator & (u128 a, u128 b)
{
  u128 c;
  c.lo = a.lo & b.lo;
  c.hi = a.hi & b.hi;
  return c;
}

force_inline u128&
operator &= (u128& a, u128 b)
{
  return a = (a & b);
}

force_inline u128 
operator * (u128 a, u128 b)
{
#if COMPILER_CL 
  {
    u128 c;

    u64 hi;
    c.lo = _umul128(a.lo, b.lo, &hi);
    
    u64 lo_1 = a.hi * b.lo;
    u64 lo_2 = a.lo * b.hi;

    c.hi = hi + lo_1 + lo_2;

    return c;
  }
#else 
#  error "not implemented."
#endif
}

struct udiv_128_64_result_t {
  u64 quotient;
  u64 remainder;
  b32 error;
};

force_inline udiv_128_64_result_t
udiv_128_64(u128 l, u64 r)
{
#if COMPILER_CL
  {
    if (l.hi >= r) {
      return { UINT64_MAX, UINT64_MAX, true };
    }

    u64 remainder;
    u64 quotient = _udiv128(l.hi, l.lo, r, &remainder);
    return { quotient, remainder, false };
  }
#else
#  error "not implemented."
#endif
}

struct div_128_128_result_t {
  s128 quotient;
  s128 remainder;
  b32  error;
};

struct udiv_128_128_result_t {
  u128 quotient;
  u128 remainder;
  b32  error;
};

force_inline udiv_128_128_result_t 
udiv_128_128(u128 u, u128 v)
{
  /*
    Code adapted from 'Hacker's Delight 2nd Edition, 9-5'.
   
    @Study
      To be honest, I have no idea what's going on in the case 
      where 'v.hi' isn't zero.
  */
  if (v.lo == 0 && v.hi == 0) {
    return { U128_ERROR, U128_ERROR, true };
  }

  if (v.hi == 0) {
    if (u.hi < v.lo) {
      // if u/v cannot overflow, just do one division.
      auto [q, r, error] = udiv_128_64(u, v.lo);

      if (error) return { U128_ERROR, U128_ERROR, true };
      else       return { { q, 0 }, { r, 0 }, false };

    } else {
      // if u/v would overflow, break u up into two halves.
      u64 q1 = u.hi / v.lo;
      u64  k = u.hi - q1*v.lo;
      u128 c = { u.lo, k };
      auto [q0, r, error] = udiv_128_64(c, v.lo);

      if (error) return { U128_ERROR, U128_ERROR, true };
      else       return { { q0, q1 }, { r, 0 }, false };
    }
  }

  u64   n = lzcnt64(v.hi); // 0 <= n <= 63
  u64  v1 = (v << n).hi;   // Normalize the divisor so its MSB is 1.
  u128 u1 = u >> 1;        // To ensure no overflow.

  auto [q1, _, error] = udiv_128_64(u1, v1);
  if (error) return { U128_ERROR, U128_ERROR, true };

  u128 q0 = ( (u128{ q1, 0 } << n) >> 63 ); // Undo normalization and division of u by 2.

  // Make q0 correct or too small by 1.
  if (q0.lo || q0.hi) {
    if (q0.lo == 0) {
      q0.hi -= 1;
    }
    q0.lo -= 1;
  }

  u128 remainder = u - q0*v;
  if (remainder >= v) {
    q0.lo += 1;
    if (q0.lo == 0) {
      q0.hi += 1; // Now q0 is correct.
    }
    remainder -= v;
  }

  return { q0, remainder, false };
}

/*
  We truncate.

  >>> TRUNCATING <<<
       MODULUS       
        FLOOR        
  
  e.g.
        7  /   3   =   2 rem  1
      (-7) /   3   =  -2 rem -1
        7  / (-3)  =  -2 rem  1
      (-7) / (-3)  =   2 rem -1
*/
force_inline div_128_128_result_t 
div_128_128(s128 u, s128 v)
{
  bool neg_quot = false;
  bool neg_rem  = false;

  if (u.hi < 0) {
    u = -u;
    neg_quot = true;
    neg_rem  = true;
  }

  if (v.hi < 0) {
    v = -v;
    neg_quot = !neg_quot;
  }

  auto [q, r, error] = udiv_128_128( { u.lo, (u64)u.hi }, { v.lo, (u64)v.hi } );
  if (error) {
    return { s128{ q.lo, (s64)q.hi }, s128{ r.lo, (s64)r.hi }, true };
  }

  s128 q_ = s128{ q.lo, (s64)q.hi };
  s128 r_  = s128{ r.lo, (s64)r.hi };

  if (neg_quot) q_ = -q_;
  if (neg_rem)  r_ = -r_;

  return { q_, r_, false };
}

force_inline u128 
operator / (u128 a, u128 b)
{
  R_ASSERT( (b.lo != 0 || b.hi != 0), "division by zero error" );
  return udiv_128_128(a, b).quotient;
}

force_inline s128 
operator / (s128 a, s128 b)
{
  R_ASSERT( (b.lo != 0 || b.hi != 0), "division by zero error" );
  return div_128_128(a, b).quotient;
}

force_inline u128&
operator /= (u128& a, u128 b)
{
  return a = a / b;
}

force_inline s128&
operator /= (s128& a, s128 b)
{
  return a = a / b;
}

force_inline s128 
operator * (s128 a, s128 b)
{
  // @Todo: better way for two's complement?
  bool negate = false;

  if (a.hi < 0) {
    negate = true;
    a = -a;
  }

  if (b.hi < 0) {
    negate = !negate;
    b = -b;
  }

  u128 u = u128{ a.lo, (u64)a.hi } * u128{ b.lo, (u64)b.hi };
  s128 c = { u.lo, (s64)u.hi };

  if (negate) c = -c;
  return c;
}




#endif // RTS_INT128_H
