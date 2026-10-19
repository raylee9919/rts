// Copyright Seong Woo Lee. All Rights Reserved.

import basic;
import material;
#include "shared/shared.h"
#include "./terrain.h"

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
    float2 uv          : UV;
    float3 position    : POSITION;
    float3 normal      : NORMAL;
    float4 tangent     : TANGENT;
};

PUSH_CONSTANTS(Terrain_Push_Constants, push);


[shader("vertex")]
VS_Output main_vs(uint vertex_id : SV_VertexID) 
{
    VS_Output result;

    StructuredBuffer<Vertex> vertex_buffer = ResourceDescriptorHeap[push.vertex_buffer_id];
    Vertex vert = vertex_buffer[vertex_id];

    StructuredBuffer<GPU_Camera> camera_buffer = ResourceDescriptorHeap[push.camera_buffer_id];
    GPU_Camera camera = camera_buffer[0];

    float4 world_position = float4(vert.position, 1.0);
    float4x4 view_proj    = mul(camera.proj, camera.view);

    result.sv_position = mul(view_proj, world_position);
    result.position    = world_position.xyz;
    result.uv          = vert.uv * 4.0;
    result.normal      = vert.normal;
    result.tangent     = vert.tangent;

    return result;
}

// @Temporary: single directional light
static const float3 TO_LIGHT        = normalize(float3(0.5, -0.5, 1.0));
static const float3 LIGHT_RADIANCE  = float3(1.0, 1.0, 1.0) * 4.0;

[shader("pixel")]
float4 main_ps(VS_Output input) : SV_TARGET 
{
    ByteAddressBuffer material_buffer = ResourceDescriptorHeap[push.material_buffer_id];
    Material material = material_buffer.Load<Material>(push.material_offset);

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
