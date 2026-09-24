// Copyright Seong Woo Lee. All Rights Reserved.

struct Push_Constants {
    uint32_t    linear_sampler_id;
    uint32_t    buffer_id;
    uint32_t    base_index;
    float       width; // Projection
    float       height;
};

struct Quad {
    uint32_t  texture_id;
    float2    position[4];
    float2    uv[4];
    float4    color[4];
};
