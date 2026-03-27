// Copyright Seong Woo Lee. All Rights Reserved.

R"(
#version 460

#extension GL_ARB_bindless_texture : require

#define u32     uint
#define s32     int
#define f32     float
#define v2      vec2
#define v3      vec3
#define v4      vec4
#define m3x3    mat3x3
#define m4x4    mat4x4
#define iv2     ivec2
#define iv3     ivec3
#define iv4     ivec4
#define uv3     uvec3
#define uv4     uvec4

#define LIGHT_ATTENUATION_CONSTANT  0.6f
#define LIGHT_ATTENUATION_LINEAR    0.3f
#define LIGHT_ATTENUATION_QUADRATIC 0.1f

#define PI32    3.14159265359
#define EPS32   1e-30
#define SQRT_3  1.73205080757

m4x4 identity() 
{
    return m4x4(1, 0, 0, 0,
                0, 1, 0, 0,
                0, 0, 1, 0,
                0, 0, 0, 1);
}

vec4 rgba8_to_v4(uint val)
{
    vec4 result = vec4(
        float((val & 0x000000FF)) / 255.0 * 2.0 - 1.0,
        float((val & 0x0000FF00) >>  8U) / 255.0 * 2.0 - 1.0,
        float((val & 0x00FF0000) >> 16U) / 255.0 * 2.0 - 1.0,
        float((val & 0xFF000000) >> 24U)
    );
    return result;
}

u32 v4_to_rgba8(v4 val)
{
    u32 result = (u32(val.w) & 0x000000FF) << 24U |
                 (u32((val.z * 0.5f + 0.5f) * 255.0f) & 0x000000FF) << 16U |
                 (u32((val.y * 0.5f + 0.5f) * 255.0f) & 0x000000FF) <<  8U |
                 (u32((val.x * 0.5f + 0.5f) * 255.0f) & 0x000000FF);
    return result;
}

f32 light_attenuation(v3 light_P, v3 world_P)
{
    f32 D = distance(light_P, world_P);
    f32 result = 1.0f / (LIGHT_ATTENUATION_CONSTANT +
                        LIGHT_ATTENUATION_LINEAR * D +
                        LIGHT_ATTENUATION_QUADRATIC * D * D);
    return result;
}

bool is_in_clip_space(v3 V)
{
    bool result = (
        V.x >= -1.0f && V.x <= 1.0f &&
        V.y >= -1.0f && V.y <= 1.0f &&
        V.z >= -1.0f && V.z <= 1.0f
    );
    return result;
}

)";
