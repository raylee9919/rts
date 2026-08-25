// Copyright Seong Woo Lee. All Rights Reserved.

#define CAMERA_DEFAULT_INDEX 0

float4 unpack_rgba8(uint packed) 
{
    return float4((packed >>  0) & 0xff,
                  (packed >>  8) & 0xff,
                  (packed >> 16) & 0xff,
                  (packed >> 24) & 0xff) / 255.0;
}

struct Push_Constants {
    uint vertex_buffer_id;
    uint texture_id;
    uint linear_sampler_id;
    uint camera_id;
    uint arguments_id;
};
ConstantBuffer<Push_Constants> push : register(b0);

struct Camera {
    float4   position;
    float4x4 view;
    float4x4 proj;
    float4x4 view_proj;
};

struct Arguments {
    float3   position;
    float4x4 orientation;
    uint     tint;
};

struct Vertex {
    float3 position;
    float3 normal;
    float2 uv;
};

struct VS_Output {
    float4 sv_position : SV_POSITION;
    float3 position    : POSITION;
    float3 normal      : NORMAL;
    float2 uv          : UV;
    float4 color       : COLOR;
};

VS_Output main_vs(uint vertex_id : SV_VertexID, uint instance_id : SV_InstanceID) 
{
    VS_Output result;

    StructuredBuffer<Vertex> vertex_buffer = ResourceDescriptorHeap[push.vertex_buffer_id];
    Vertex vert = vertex_buffer[vertex_id];

    StructuredBuffer<Arguments> arguments_buffer = ResourceDescriptorHeap[push.arguments_id];
    Arguments args = arguments_buffer[instance_id];
    float3 translation = args.position;

    StructuredBuffer<Camera> camera_buffer = ResourceDescriptorHeap[push.camera_id];
    Camera camera = camera_buffer[CAMERA_DEFAULT_INDEX];

    float3 position = vert.position + translation;

    float4x4 view_proj = mul(camera.proj, camera.view);

    result.sv_position = mul(view_proj, float4(position, 1.0));
    result.position    = position;
    result.uv          = vert.uv;
    result.normal      = vert.normal;
    result.color       = unpack_rgba8(args.tint);

    return result;
}

float4 main_ps(VS_Output input) : SV_TARGET 
{
    // Texture2D <float4> color_texture  = ResourceDescriptorHeap[push.texture_id];
    // SamplerState linear_sampler = SamplerDescriptorHeap[push.linear_sampler_id];
    // float4 result = color_texture.Sample(linear_sampler, input.uv) * input.color;

    float4 result = float4(input.normal, 1.0);

    return result;
}
