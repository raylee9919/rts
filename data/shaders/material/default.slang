// Copyright Seong Woo Lee. All Rights Reserved.

module material_default;

import basic;
import material;
#include "shared/shared.h"

struct M_Default : IMaterial
{
    /* Fields */
    [Texture] uint32_t albedo_id;
    [Texture] uint32_t normal_id;
    [Texture] uint32_t roughness_id;
    [Texture] uint32_t metallic_id;


    /* Evaluation Implementation */
    float4 eval( Material_Input input, float3 radiance, float3 wi, float3 wo )
    {
        SamplerState linear_sampler = SamplerDescriptorHeap[input.linear_sampler_id];

        float3 albedo    = 1.0;
        float  roughness = 1.0;
        float  metallic  = 0.0;
        float3 n         = float3(0.0, 0.0, 1.0);

        if ( albedo_id != GFX_INVALID_BINDLESS ) {
            Texture2D<float4> t = ResourceDescriptorHeap[NonUniformResourceIndex(albedo_id)];
            albedo = linear_from_sRGB( t.Sample(linear_sampler, input.uv).rgb );
        }
        
        if ( roughness_id != GFX_INVALID_BINDLESS ) {
            Texture2D<float4> t = ResourceDescriptorHeap[NonUniformResourceIndex(roughness_id)];
            roughness = t.Sample(linear_sampler, input.uv).r;
        }

        if ( metallic_id != GFX_INVALID_BINDLESS ) {
            Texture2D<float4> t = ResourceDescriptorHeap[NonUniformResourceIndex(metallic_id)];
            metallic = t.Sample(linear_sampler, input.uv).r;
        }

        if ( normal_id != GFX_INVALID_BINDLESS ) {
            Texture2D<float4> t = ResourceDescriptorHeap[NonUniformResourceIndex(normal_id)];
            n = t.Sample(linear_sampler, input.uv).rgb * 2.0 - 1.0;
        }

        // Tangent space to world
        float3 N = normalize(input.normal);
        float3 T = normalize(input.tangent.xyz - dot(input.tangent.xyz, N) * N);
        float3 B = input.tangent.w * cross(N, T);
        n = normalize(T * n.x + B * n.y + N * n.z);

        float3 result = brdf_cook_torrance(n, wi, wo, albedo, roughness, metallic) * radiance;
        return float4(result, 1.0);
    }
};

export struct Material : IMaterial = M_Default;
