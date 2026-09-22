// Copyright Seong Woo Lee. All Rights Reserved.

import basic;
import material;
#include "shared/shared.h"
#include "./surface.h"

struct Instance {
    float4x4 transform;
    uint32_t material_id;
};

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
    float2 uv          : UV;

    nointerpolation uint32_t material_id : MATERIAL_ID;
};

PUSH_CONSTANTS(Push_Constants, push);

extern struct Material : IMaterial; // Slang links with corresponding material implementation.

[shader("vertex")]
VS_Output main(uint vertex_id : SV_VertexID, uint instance_id : SV_InstanceID) 
{
    VS_Output result;

    StructuredBuffer<Vertex> vertex_buffer = ResourceDescriptorHeap[push.vertex_buffer_id];
    Vertex vert = vertex_buffer[vertex_id];

    StructuredBuffer<Instance> argument_buffer = ResourceDescriptorHeap[push.argument_buffer_id];
    Instance inst = argument_buffer[push.argument_base_index + instance_id];

    StructuredBuffer<GPU_Camera> camera_buffer = ResourceDescriptorHeap[push.camera_buffer_id];
    GPU_Camera camera = camera_buffer[0];

    float4 position = mul(inst.transform, float4(vert.position, 1.0));

    float4x4 view_proj = mul(camera.proj, camera.view);

    result.sv_position = mul(view_proj, position);
    result.position    = position.xyz;
    result.uv          = vert.uv;
    result.normal      = vert.normal;
    result.material_id = inst.material_id;

    return result;
}

[shader("fragment")]
float4 main(VS_Output input) : SV_TARGET 
{
    ByteAddressBuffer material_buffer = ResourceDescriptorHeap[push.material_buffer_id];
    Material material = material_buffer.Load<Material>(input.material_id);

    Material_Input mtl;
    mtl.position          = input.position;
    mtl.normal            = input.normal;
    mtl.uv                = input.uv;
    mtl.linear_sampler_id = push.linear_sampler_id;

    float4 result = material.eval(mtl, 1.0, 1.0);
    return result;
}
