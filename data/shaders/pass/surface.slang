// Copyright Seong Woo Lee. All Rights Reserved.

import basic;
import material;
#include "shared/shared.h"
#include "./surface.h"

extern struct Material : IMaterial;

struct Vertex 
{
    float3 position;
    float2 uv;
    float3 normal;
    float4 tangent;
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

    float4 result = material.eval(mtl, LIGHT_RADIANCE, wi, wo);
    return result;
}
