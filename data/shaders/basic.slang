// Copyright Seong Woo Lee. All Rights Reserved.

module basic;

public typealias vec3 = float3;
public typealias vec4 = float4;
public typealias m4x4 = float4x4;

public float4 unpack_rgba8(uint packed) 
{
    return float4((packed >>  0) & 0xff,
                  (packed >>  8) & 0xff,
                  (packed >> 16) & 0xff,
                  (packed >> 24) & 0xff) / 255.0;
}


//
// Linear/sRGB conversion
//
public float3 sRGB_from_linear(float3 c) 
{
    // @Note: iirc, it's from Frostbite engine?
    float3 lo = c * 12.92;
    float3 hi = (pow(abs(c), 1.0/2.4) * 1.055) - 0.055;
    float3 srgb = select(c <= 0.0031308, lo, hi);
    return srgb;
}

public float3 linear_from_sRGB(float3 c)
{
    return pow(c, 2.2);
}
