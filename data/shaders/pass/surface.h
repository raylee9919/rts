// Copyright Seong Woo Lee. All Rights Reserved.

struct Push_Constants {
    uint32_t        vertex_buffer_id;
    uint32_t        linear_sampler_id;
    uint32_t        camera_buffer_id;
    uint32_t        arguments_buffer_id;
    uint32_t        arguments_index;
    uint32_t        material_buffer_id;
};

struct Arguments {
    float           transform[4][4];
    uint32_t        material_id;
};
