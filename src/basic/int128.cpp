// Copyright Seong Woo Lee. All Rights Reserved.

#include "./int128.h"

udiv_128_128_result_t 
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
