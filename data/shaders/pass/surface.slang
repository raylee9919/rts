// Copyright Seong Woo Lee. All Rights Reserved.

import basic;
import material;
#include "shared/shared.h"
#include "./surface.h"

extern struct Material : IMaterial;

// @Cleanup
struct Vertex 
{
    float3 position;
    float2 uv;
    float3 normal;
    float4 tangent;
};

struct Skinned_Vertex 
{
    float3 position;
    float2 uv;
    float3 normal;
    float4 tangent;
    int4   joints;
    float4 weights;
};

struct VS_Output 
{
    float4 sv_position : SV_Position;
    float3 position    : POSITION;
    float3 normal      : NORMAL;
    float4 tangent     : TANGENT;
    float2 uv          : UV;

    nointerpolation uint32_t material_id : MATERIAL_ID;
};

PUSH_CONSTANTS(Push_Constants, push);


[shader("vertex")]
VS_Output main_vs_static(uint vertex_id : SV_VertexID, uint instance_id : SV_InstanceID) 
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

[shader("vertex")]
VS_Output main_vs_skeletal(uint vertex_id : SV_VertexID, uint instance_id : SV_InstanceID)
{
    VS_Output result;

    StructuredBuffer<Skinned_Vertex> vertex_buffer = ResourceDescriptorHeap[push.vertex_buffer_id];
    Skinned_Vertex vert = vertex_buffer[vertex_id];

    StructuredBuffer<Arguments> argument_buffer = ResourceDescriptorHeap[push.argument_buffer_id];
    Arguments inst = argument_buffer[push.argument_base_index + instance_id];

    StructuredBuffer<GPU_Camera> camera_buffer = ResourceDescriptorHeap[push.camera_buffer_id];
    GPU_Camera camera = camera_buffer[0];

    StructuredBuffer<float3x4> skinning_buffer = ResourceDescriptorHeap[push.skinning_buffer_id];

    // Blend joint matrices (linear blend skinning).
    uint  base = inst.skinning_matrices;
    uint4 j    = min(uint4(vert.joints), inst.num_matrices - 1);   // guard against bad indices
    float4 w   = vert.weights;

    float3x4 skin = w.x * skinning_buffer[base + j.x]
                  + w.y * skinning_buffer[base + j.y]
                  + w.z * skinning_buffer[base + j.z]
                  + w.w * skinning_buffer[base + j.w];

    // Bind-pose model space -> current model space
    float3 model_position = mul(skin, float4(vert.position,    1.0));
    float3 model_normal   = mul(skin, float4(vert.normal,      0.0));
    float3 model_tangent  = mul(skin, float4(vert.tangent.xyz, 0.0));

    // Model -> world -> clip (unchanged)
    float4 position = mul(inst.transform, float4(model_position, 1.0));

    float4x4 view_proj = mul(camera.proj, camera.view);

    result.sv_position = mul(view_proj, position);
    result.position    = position.xyz;
    result.uv          = vert.uv;
    result.normal      = normalize(mul(inst.transform, float4(model_normal,  0.0)).xyz);
    result.tangent     = float4(normalize(mul(inst.transform, float4(model_tangent, 0.0)).xyz), vert.tangent.w);
    result.material_id = inst.material_index;

    return result;
}

// @Temporary: single directional light
static const float3 TO_LIGHT        = normalize(float3(0.5, 1.0, 0.8));
static const float3 LIGHT_RADIANCE  = float3(1.0, 1.0, 1.0) * 4.0;

[shader("pixel")]
float4 main_ps(VS_Output input) : SV_TARGET 
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

    float4 result = material.eval(mtl, LIGHT_RADIANCE, wi, wo);
    return result;
}
