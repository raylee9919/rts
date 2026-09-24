// Copyright Seong Woo Lee. All Rights Reserved.

import basic;
import material;
#include "shared/shared.h"
#include "./ui.h"

PUSH_CONSTANTS(Push_Constants, push);

struct VS_Output {
    float4 clip_position : SV_POSITION;
    float2 uv            : UV;
    float4 color         : COLOR;
    uint32_t texture_id  : TEXTURE_ID;
};

[shader("vertex")]
VS_Output main_vs(uint vid : SV_VertexID, 
                  uint iid : SV_InstanceID) 
{
    StructuredBuffer<Quad> quads = ResourceDescriptorHeap[push.buffer_id];
    Quad quad = quads[push.base_index + iid];

    float2 pos   = quad.position[vid];
    float2 uv    = quad.uv[vid];
    float4 color = quad.color[vid];

    float x = (( 2.0 * pos.x) / push.width)  - 1.0;
    float y = ((-2.0 * pos.y) / push.height) + 1.0;

    VS_Output result;
    result.clip_position = float4(x, y, 0.0, 1.0);
    result.uv            = uv;
    result.color         = color;
    result.texture_id    = quad.texture_id;

    return result;
}

[shader("fragment")]
float4 main_fs(VS_Output input) : SV_TARGET 
{
    float4 result = input.color;

    SamplerState linear_sampler = SamplerDescriptorHeap[push.linear_sampler_id];

    if (input.texture_id != GFX_INVALID_BINDLESS) {
        Texture2D<float4> tex = ResourceDescriptorHeap[input.texture_id];
        result *= tex.Sample( linear_sampler, input.uv );
    }

    return result;
}
