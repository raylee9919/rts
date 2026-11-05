// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RTS_INT128_H
#define RTS_INT128_H

#include "basic/core.h"

struct s128 {
  u64 lo;
  s64 hi;
};

struct u128 {
  u64 lo;
  u64 hi;
};

force_inline bool 
operator == (s128 a, s128 b)
{
  return (a.lo == b.lo) && (a.hi == b.hi);
}

force_inline bool 
operator == (u128 a, u128 b)
{
  return (a.lo == b.lo) && (a.hi == b.hi);
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
    // @Speed: It might be better to write compiler specific code!
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
  // Don't worry! It produces just as good code as u128 one.
  // (https://godbolt.org/z/vP8T15s9j)
  u128 c = u128{a.lo, (u64)a.hi} + u128{b.lo, (u64)b.hi};
  return s128{c.lo, (s64)c.hi};
}


#endif // RTS_INT128_H
