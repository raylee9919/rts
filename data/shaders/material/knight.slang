// Copyright Seong Woo Lee. All Rights Reserved.

module material_knight;

import basic;
import material;
#include "shared/shared.h"

// Albedo only for now. Normal / roughness / metallic maps are cooked and sitting
// in data/textures, they just need fields here and lighting to consume them.
struct Mtl_Knight : IMaterial 
{
    /* Fields */
    [Texture] uint32_t albedo_id;


    /* Evaluation Implementation */
    float4 eval( Material_Input input, float3 wi, float3 wo )
    {
        SamplerState linear_sampler = SamplerDescriptorHeap[input.linear_sampler_id];

        float3 albedo = 1.0;

        if ( albedo_id != GFX_INVALID_BINDLESS ) 
        {
            Texture2D<float4> albedo_texture = ResourceDescriptorHeap[NonUniformResourceIndex(albedo_id)];
            albedo = albedo_texture.Sample(linear_sampler, input.uv).rgb;
        }

        return float4(albedo, 1.0);
    }
};

// Resolves `extern struct Material` in the shader this gets linked with.
export struct Material : IMaterial = Mtl_Knight;
