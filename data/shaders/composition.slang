// Copyright Seong Woo Lee. All Rights Reserved.

import basic;
#include "shared/shared.h"

PUSH_CONSTANTS(PC_Composition, push);

struct VS_Output {
    float4 sv_position : SV_Position;
    float2 screen_uv   : TEXCOORD0;
};

[shader("vertex")]
VS_Output vs_main(uint vertex_id : SV_VertexID)
{
    VS_Output result;
    float2 pos  = float2(((vertex_id & 1) != 0) ?  3.0 : -1.0,
                         ((vertex_id & 2) != 0) ? -3.0 :  1.0);
    result.sv_position = float4(pos, 0.0, 1.0);
    result.screen_uv   = float2(((vertex_id & 1) != 0) ? 2.0 : 0.0,
                                ((vertex_id & 2) != 0) ? 2.0 : 0.0);
    return result;
}

[shader("fragment")]
float4 ps_main(VS_Output input) : SV_TARGET
{
    SamplerState dot_sampler = SamplerDescriptorHeap[push.dot_sampler_id];

    Texture2D<float4> scene_texture = ResourceDescriptorHeap[push.scene_texture_id];
    float4 texel = scene_texture.Sample(dot_sampler, input.screen_uv);

    float4 result = texel;
    return result;
}
