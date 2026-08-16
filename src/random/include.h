// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RTS_RANDOM_H
#define RTS_RANDOM_H


// @Todo: You might want this in the game state. Your temporary render tick 
// can break the determinism!
per_thread u64 xorshift_state_64 = 0xCafeBabe;
per_thread u32 xorshift_state_32 = 0xDeadBeef;

internal u64 xorshift64();
internal u32 xorshift32();


#endif // RTS_RANDOM_H
