// Copyright Seong Woo Lee. All Rights Reserved.

#define MAIN_CAMERA_INDEX 0

#include "./basic.h"
#include "./shared.h"

struct Push_Constants {
    uint32_t vertex_buffer_id;
    uint32_t linear_sampler_id;
    uint32_t camera_buffer_id;
    uint32_t arguments_id;
    uint32_t material_buffer_id;
};
PUSH_CONSTANTS(Push_Constants);

struct Camera {
    float4   position;
    float4x4 view;
    float4x4 proj;
    float4x4 view_proj;
};

struct Instance {
    float4x4 transform;
    uint32_t material_id;
};

struct Vertex {
    float3 position;
    float3 normal;
    float2 uv;
};

struct VS_Output {
    float4      sv_position : SV_POSITION;
    float3      position    : POSITION;
    float3      normal      : NORMAL;
    float2      uv          : UV;

    uint32_t    material_id : MATERIAL_ID;
};

VS_Output main_vs(uint vertex_id : SV_VertexID, uint instance_id : SV_InstanceID) 
{
    VS_Output result;

    StructuredBuffer<Vertex> vertex_buffer = ResourceDescriptorHeap[push.vertex_buffer_id];
    Vertex vert = vertex_buffer[vertex_id];

    StructuredBuffer<Instance> arguments_buffer = ResourceDescriptorHeap[push.arguments_id];
    Instance inst = arguments_buffer[instance_id];

    StructuredBuffer<Camera> camera_buffer = ResourceDescriptorHeap[push.camera_buffer_id];
    Camera camera = camera_buffer[MAIN_CAMERA_INDEX];

    float4 position = mul(inst.transform, float4(vert.position, 1.0));

    float4x4 view_proj = mul(camera.proj, camera.view);

    result.sv_position = mul(view_proj, position);
    result.position    = position.xyz;
    result.uv          = vert.uv;
    result.normal      = vert.normal;
    result.material_id = inst.material_id;

    return result;
}

float4 main_ps(VS_Output input) : SV_TARGET 
{
    SamplerState linear_sampler = SamplerDescriptorHeap[push.linear_sampler_id];

    StructuredBuffer<GPU_Material> material_buffer = ResourceDescriptorHeap[push.material_buffer_id];
    GPU_Material material = material_buffer[input.material_id];

    float3 albedo = 1.0;
    float  alpha  = 1.0;

    if (material.albedo_id != GFX_INVALID_BINDLESS) {
        Texture2D <float4> albedo_texture = ResourceDescriptorHeap[material.albedo_id];
        float4 texel = albedo_texture.Sample(linear_sampler, input.uv);
        albedo = texel.rgb;
        alpha  = texel.a;
    }

    float4 tint = unpack_rgba8(material.tint);
    float4 result = float4(albedo, alpha) * tint;

    return result;
}
