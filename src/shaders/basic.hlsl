// Copyright Seong Woo Lee. All Rights Reserved.

typedef float3      v3;
typedef float4      v4;
typedef float4x4    m4x4;

float4 unpack_rgba8(uint packed) {
    return float4((packed >>  0) & 0xff,
                  (packed >>  8) & 0xff,
                  (packed >> 16) & 0xff,
                  (packed >> 24) & 0xff) / 255.0;
}
