// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RTS_SHADERS_SHARED_H
#define RTS_SHADERS_SHARED_H

#if !SCOPE_SHADER
#  include "math/math.h"
#endif

#define GFX_INVALID_BINDLESS            0

#define GFX_CONSTANTS_INDEX_GLOBAL      0
#define GFX_CONSTANTS_INDEX_USER        1

#if SCOPE_SHADER
#  define CONCAT_(A, B) A##B
#  define CONCAT(A, B) CONCAT_(A, B)
#  define PUSH_CONSTANTS(Struct, Name) ConstantBuffer<Struct> Name : register(CONCAT(b, GFX_CONSTANTS_INDEX_USER))
#endif

// struct GPU_Global {
//     float time;
// };
// #if SCOPE_SHADER
//   ConstantBuffer<GPU_Global> global : register(b0);
// #endif

struct GPU_Camera {
    vec4   position;
    m4x4 view;
    m4x4 proj;
    m4x4 view_proj;
};

struct GPU_Material {
    vec3            albedo;
    float           metallic;
    float           roughness;

    uint32_t        albedo_id;
    uint32_t        orm_id;
};

struct Constants {
    uint32_t        vertex_buffer_id;
    uint32_t        linear_sampler_id;
    uint32_t        camera_buffer_id;
    uint32_t        arguments_buffer_id;
    uint32_t        arguments_index;
    uint32_t        material_buffer_id;
};

struct PC_Composition {
    uint32_t        dot_sampler_id;
    uint32_t        scene_texture_id;
};

struct Arguments {
    float           transform[4][4];
    uint32_t        material_id;
};

#endif // RTS_SHADERS_SHARED_H
