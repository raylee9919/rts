// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RTS_INT128_H
#define RTS_INT128_H

// Generated code:
// https://godbolt.org/z/7Wq8EefYf

#include "basic/core.h"

struct u128 {
  u64 lo;
  u64 hi;
};

struct s128 {
  u64 lo;
  s64 hi;
};

force_inline bool 
operator == (u128 a, u128 b)
{
  return (a.lo == b.lo) && (a.hi == b.hi);
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

force_inline u128 
operator & (u128 a, u128 b)
{
  u128 c;
  c.lo = a.lo & b.lo;
  c.hi = a.hi & b.hi;
  return c;
}

force_inline u128 
operator * (u128 a, u128 b)
{
#if COMPILER_CL 
  // @Speed: I believe there's a room for improvement!
  {
    u128 c;

    u64 hi;
    c.lo = _umul128(a.lo, b.lo, &hi);
    
    /*
    u64 unused;
    u64 lo_1 = _umul128(a.hi, b.lo, &unused);
    u64 lo_2 = _umul128(a.lo, b.hi, &unused);
    */

    u64 lo_1 = a.hi * b.lo;
    u64 lo_2 = a.lo * b.hi;

    c.hi = hi + lo_1 + lo_2;

    return c;
  }
#else 
  {
    static_assert(!"Not implemented"); // @Todo
  }
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
udiv_128_128(u128 l, u128 r)
{
  // @Todo
}

force_inline div_128_128_result_t 
div_128_128(u128 l, u128 r)
{
  // @Todo
}

force_inline bool 
operator == (s128 a, s128 b)
{
  return (a.lo == b.lo) && (a.hi == b.hi);
}

force_inline s128 
operator + (s128 a, s128 b)
{
  u128 c = u128{a.lo, (u64)a.hi} + u128{b.lo, (u64)b.hi};
  return s128{c.lo, (s64)c.hi};
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

force_inline s128 
operator * (s128 a, s128 b)
{
  // @Todo: turns out there's a better way for two's complement?
  bool negate = false;

  if (a.hi < 0) {
    negate = true;
    a = -a;
  }

  if (b.hi < 0) {
    negate = !negate;
    b = -b;
  }

  u128 u = ( (*(u128*)&a) * (*(u128*)&b) );
  s128 c = *(s128*)&u;

  if (negate) c = -c;
  return c;
}




#endif // RTS_INT128_H
