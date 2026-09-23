// Copyright Seong Woo Lee. All Rights Reserved.

struct Push_Constants {
    uint32_t        vertex_buffer_id;
    uint32_t        linear_sampler_id;
    uint32_t        camera_buffer_id;
    uint32_t        argument_buffer_id;
    uint32_t        argument_base_index;
    uint32_t        material_buffer_id;
    uint32_t        skinning_buffer_id;
};

struct Arguments {
    float4x4        transform;
    uint32_t        material_index;
    uint32_t        skinning_base_index;
    uint32_t        num_joints; // 0 if not skinned
};
