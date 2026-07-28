// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RTS_HASH_H
#define RTS_HASH_H


#define HASH_INIT 5381


//
// Knuth multiplicative 64-bit hash. Should be down-shifted to desired
// range rather than masked.
//
static u64 knuth_hash(u64 x);


static u32 default_hash(u32 x, u32 h = HASH_INIT);



#endif // RTS_HASH_H
