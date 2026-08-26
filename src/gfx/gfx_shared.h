// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RTS_GFX_SHARED_H
#define RTS_GFX_SHARED_H


#define INVALID_BINDLESS_ID     0


struct Shader_Material {
    u32         albedo_id;
    u32         orm_id;
    u32         tint;
};

struct Constants {
    u32         vertex_buffer_id;
    u32         linear_sampler_id;
    u32         camera_id;
    u32         arguments_id;
    u32         material_buffer_id;
};

struct Arguments {
    f32         position[3];
    f32         orientation[4][4];
    u32         material_id;
};


#endif // RTS_GFX_SHARED_H
