// Copyright Seong Woo Lee. All Rights Reserved.

module basic;

public typealias vec3 = float3;
public typealias vec4 = float4;
public typealias m4x4 = float4x4;

float4 unpack_rgba8(uint packed) {
    return float4((packed >>  0) & 0xff,
                  (packed >>  8) & 0xff,
                  (packed >> 16) & 0xff,
                  (packed >> 24) & 0xff) / 255.0;
}
