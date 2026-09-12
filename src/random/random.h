// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RTS_RANDOM_H
#define RTS_RANDOM_H

#include "basic/core.h"


// @Todo: You might want this in the game state. Your temporary render tick
// can break the determinism!
extern per_thread u64 xorshift_state_64;
extern per_thread u32 xorshift_state_32;

u64 xorshift64();
u32 xorshift32();


#endif // RTS_RANDOM_H
