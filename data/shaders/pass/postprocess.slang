// Copyright Seong Woo Lee. All Rights Reserved.

import basic;
#include "shared/shared.h"
#include "./postprocess.h"

PUSH_CONSTANTS(Push_Constants, push);

struct VS_Output {
    float4 sv_position : SV_Position;
    float2 screen_uv   : TEXCOORD0;
};

//
// Tone mapping functions
//
float3 tone_mapping_reinhard(float3 c)
{
    return c / (c + 1.0);
}

float3 tone_mapping_jodie_reinhard(float3 c) 
{
    // https://www.shadertoy.com/view/tdSXzD
    float luma = dot(c, float3(0.2126, 0.7152, 0.0722));
    float3 tc = c / (c + 1.0);
    return lerp(c / (luma + 1.0), tc, tc);
}

float3 tone_mapping_aces(float3 x) 
{
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return saturate((x*(a*x+b))/(x*(c*x+d)+e));
}


[shader("vertex")]
VS_Output main_vs(uint vertex_id : SV_VertexID)
{
    VS_Output result;
    float2 pos  = float2(((vertex_id & 1) != 0) ?  3.0 : -1.0,
                         ((vertex_id & 2) != 0) ? -3.0 :  1.0);
    result.sv_position = float4(pos, 0.0, 1.0);
    result.screen_uv   = float2(((vertex_id & 1) != 0) ? 2.0 : 0.0,
                                ((vertex_id & 2) != 0) ? 2.0 : 0.0);
    return result;
}

[shader("pixel")]
float4 main_ps(VS_Output input) : SV_TARGET
{
    SamplerState dot_sampler = SamplerDescriptorHeap[push.dot_sampler_id];
    Texture2D<float4> scene_texture = ResourceDescriptorHeap[push.scene_texture_id];
    float4 c = scene_texture.Sample(dot_sampler, input.screen_uv);

    // Tone-mapping and linear to sRGB conversion
    float3 color = tone_mapping_jodie_reinhard(c.rgb);
    color = sRGB_from_linear(color);

    float4 result = float4(color, c.a);
    return result;
}
