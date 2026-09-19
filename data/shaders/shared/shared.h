// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RTS_SHADERS_SHARED_H
#define RTS_SHADERS_SHARED_H

#ifndef __SLANG__
#  include "math/math.h"
#endif

#define GFX_INVALID_BINDLESS            0
#define GFX_CONSTANTS_INDEX_GLOBAL      0
#define GFX_CONSTANTS_INDEX_USER        1

#ifdef __SLANG__
#  define CONCAT_(A, B) A##B
#  define CONCAT(A, B) CONCAT_(A, B)
#  define PUSH_CONSTANTS(Struct, Name) ConstantBuffer<Struct> Name : register(CONCAT(b, GFX_CONSTANTS_INDEX_USER))
#endif

#if 0
struct GPU_Global {
    float time;
};
#ifdef __SLANG__
  ConstantBuffer<GPU_Global> global : register(b0);
#endif
#endif

struct GPU_Camera {
    vec4            position;
    m4x4            view;
    m4x4            proj;
    m4x4            view_proj;
};

struct GPU_Material {
    vec3            albedo;
    float           metallic;
    float           roughness;

    uint32_t        albedo_id;
    uint32_t        orm_id;
};

#endif // RTS_SHADERS_SHARED_H
