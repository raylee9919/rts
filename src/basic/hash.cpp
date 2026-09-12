// Copyright Seong Woo Lee. All Rights Reserved.

#include "basic/hash.h"

u64 knuth_hash(u64 x) {
    u64 KNUTH_GOLDEN_RATIO_64 = 11400714819323198485;
    return KNUTH_GOLDEN_RATIO_64 * x;
}

u32 default_hash(u32 x, u32 h) {
    return (u32)(knuth_hash(((u64)x) ^ h) >> 32);
}
