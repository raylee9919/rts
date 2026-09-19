// Copyright Seong Woo Lee. All Rights Reserved.

module material_doggo;

import basic;
import material;
#include "shared/shared.h"

// @Temporary 
// Layout must match GPU_Material until the C++ side packs per-material structs.
struct Mtl_Doggo : IMaterial {
    float3   albedo;
    float    metallic;
    float    roughness;

    uint32_t albedo_id;
    uint32_t orm_id;

    float4 eval( Material_Input input, float3 wi, float3 wo )
    {
        SamplerState linear_sampler = SamplerDescriptorHeap[input.linear_sampler_id];

        float3 albedo = this.albedo;
        float  alpha  = 1.0;

        if (albedo_id != GFX_INVALID_BINDLESS) 
        {
            Texture2D<float4> albedo_texture = ResourceDescriptorHeap[NonUniformResourceIndex(albedo_id)];
            float4 texel = albedo_texture.Sample(linear_sampler, input.uv);
            albedo = texel.rgb * albedo;
            alpha  = texel.a;
        }

        float4 result = float4(albedo, alpha);
        return result;
    }
};

// Resolves `extern struct Material` in the shader this gets linked with.
export struct Material : IMaterial = Mtl_Doggo;
