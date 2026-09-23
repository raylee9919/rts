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
// Tone mapping functions
//
public float3 tone_mapping_reinhard(float3 c)
{
    return c / (c + 1.0);
}

public float3 tone_mapping_jodie_reinhard(float3 c) 
{
    // https://www.shadertoy.com/view/tdSXzD
    float luma = dot(c, float3(0.2126, 0.7152, 0.0722));
    float3 tc = c / (c + 1.0);
    return lerp(c / (luma + 1.0), tc, tc);
}

public float3 tone_mapping_aces(float3 x) 
{
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return saturate((x*(a*x+b))/(x*(c*x+d)+e));
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
