// Copyright Seong Woo Lee. All Rights Reserved.

import basic;
import material;
#include "shared/shared.h"
#include "./surface.h"

// Slang links with corresponding material implementation.
extern struct Material : IMaterial;

// Mirrors Asset::Vertex in src/asset/mesh.h. Keep the two in sync.
// Every 4-component vector is 16-byte aligned, so this lays out identically under
// scalar and cbuffer-style packing. int4/float4 rather than arrays for the same reason.
struct Vertex {
    float4 tangent;      // 0
    float3 position;     // 16
    uint   color;        // 28
    float3 normal;       // 32
    float  _pad0;        // 44
    float4 node_weights; // 48
    int4   node_ids;     // 64
    float2 uv;           // 80
    float2 _pad1;        // 88
};

struct VS_Output {
    float4 sv_position : SV_Position;
    float3 position    : POSITION;
    float3 normal      : NORMAL;
    float4 tangent     : TANGENT;
    float2 uv          : UV;

    nointerpolation uint32_t material_id : MATERIAL_ID;
};

PUSH_CONSTANTS(Push_Constants, push);


[shader("vertex")]
VS_Output main(uint vertex_id : SV_VertexID, uint instance_id : SV_InstanceID) 
{
    VS_Output result;

    StructuredBuffer<Vertex> vertex_buffer = ResourceDescriptorHeap[push.vertex_buffer_id];
    Vertex vert = vertex_buffer[vertex_id];

    StructuredBuffer<Arguments> argument_buffer = ResourceDescriptorHeap[push.argument_buffer_id];
    Arguments inst = argument_buffer[push.argument_base_index + instance_id];

    StructuredBuffer<GPU_Camera> camera_buffer = ResourceDescriptorHeap[push.camera_buffer_id];
    GPU_Camera camera = camera_buffer[0];

    float3 model_position = vert.position;
    float3 model_normal   = vert.normal;
    float3 model_tangent  = vert.tangent.xyz;

    if (inst.num_joints > 0) {
        StructuredBuffer<float3x4> skinning_buffer = ResourceDescriptorHeap[push.skinning_buffer_id];

        float3 skinned_position = 0.0;
        float3 skinned_normal   = 0.0;
        float3 skinned_tangent  = 0.0;
        float  weights_sum      = 0.0;

        for (int i = 0; i < 4 && vert.node_ids[i] >= 0; ++i) {
            float3x4 m = skinning_buffer[inst.skinning_base_index + vert.node_ids[i]];
            float    w = vert.node_weights[i];

            skinned_position += w * mul(m, float4(vert.position, 1.0));
            skinned_normal   += w * mul(m, float4(vert.normal,   0.0));
            skinned_tangent  += w * mul(m, float4(vert.tangent.xyz, 0.0));
            weights_sum      += w;
        }

        if (weights_sum > 1e-8) {
            model_position = skinned_position / weights_sum;
            model_normal   = normalize(skinned_normal);
            model_tangent  = normalize(skinned_tangent);
        }
    }

    float4 position = mul(inst.transform, float4(model_position, 1.0));

    float4x4 view_proj = mul(camera.proj, camera.view);

    result.sv_position = mul(view_proj, position);
    result.position    = position.xyz;
    result.uv          = vert.uv;
    result.normal      = mul(inst.transform, float4(model_normal,  0.0)).xyz;
    result.tangent     = float4(mul(inst.transform, float4(model_tangent, 0.0)).xyz, vert.tangent.w);
    result.material_id = inst.material_index;

    return result;
}

// @Temporary: single directional light
static const float3 TO_LIGHT        = normalize(float3(0.5, 1.0, 0.8));
static const float3 LIGHT_RADIANCE  = float3(1.0, 1.0, 1.0) * 4.0;

[shader("fragment")]
float4 main(VS_Output input) : SV_TARGET 
{
    ByteAddressBuffer material_buffer = ResourceDescriptorHeap[push.material_buffer_id];
    Material material = material_buffer.Load<Material>(input.material_id);

    StructuredBuffer<GPU_Camera> camera_buffer = ResourceDescriptorHeap[push.camera_buffer_id];
    GPU_Camera camera = camera_buffer[0];

    Material_Input mtl;
    mtl.position          = input.position;
    mtl.normal            = input.normal;
    mtl.tangent           = input.tangent;
    mtl.uv                = input.uv;
    mtl.linear_sampler_id = push.linear_sampler_id;

    float3 wi = TO_LIGHT;
    float3 wo = normalize(camera.position.xyz - input.position);

    float4 result = material.eval(mtl, wi, wo);
    result.rgb *= LIGHT_RADIANCE;
    return result;
}
