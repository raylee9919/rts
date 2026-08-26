// Copyright Seong Woo Lee. All Rights Reserved.

#define GFX_INVALID_BINDLESS            0

#define GFX_CONSTANTS_INDEX_GLOBAL      0
#define GFX_CONSTANTS_INDEX_USER        1

#if SCOPE_SHADER
#  define CONCAT_(A, B) A##B
#  define CONCAT(A, B) CONCAT_(A, B)
#  define PUSH_CONSTANTS(Struct) ConstantBuffer<Struct> push : register(CONCAT(b, GFX_CONSTANTS_INDEX_USER))
#endif

struct GPU_Global {
    float time;
};
#if SCOPE_SHADER
ConstantBuffer<GPU_Global> global : register(b0);
#endif

struct GPU_Camera {
    v4   position;
    m4x4 view;
    m4x4 proj;
    m4x4 view_proj;
};

struct GPU_Material {
    uint32_t        albedo_id;
    uint32_t        orm_id;
    uint32_t        tint;
};

struct Constants {
    uint32_t        vertex_buffer_id;
    uint32_t        linear_sampler_id;
    uint32_t        camera_id;
    uint32_t        arguments_id;
    uint32_t        material_buffer_id;
};

struct Arguments {
    float           transform[4][4];
    uint32_t        material_id;
};
