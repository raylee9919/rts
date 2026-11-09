// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RTS_INT128_H
#define RTS_INT128_H

// Generated code:
// https://godbolt.org/z/7Wq8EefYf

#include "basic/core.h"

//
// u128
//
struct u128 {
  u64 lo;
  u64 hi;
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
  // @Speed: I believe there's a room of improvement!
  {
    u128 c;

    u64 hi;
    c.lo = _umul128(a.lo, b.lo, &hi);

    u64 unused;
    u64 lo_1 = _umul128(a.hi, b.lo, &unused);
    u64 lo_2 = _umul128(a.lo, b.hi, &unused);

    c.hi = hi + lo_1 + lo_2;

    return c;
  }
#else 
  {

  }
#endif
}


//
// s128
//
struct s128 {
  u64 lo;
  s64 hi;
};

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




#endif // RTS_INT128_H
