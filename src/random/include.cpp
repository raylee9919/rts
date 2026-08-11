// Copyright Seong Woo Lee. All Rights Reserved.

u64 xorshift64() {
    u64 x = xorshift_state_64;

    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;

    xorshift_state_64 = x;
    return x;
}

u32 xorshift32() {
    u32 x = xorshift_state_32;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    xorshift_state_32 = x;
    return x;
}
