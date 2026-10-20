// Copyright Seong Woo Lee. All Rights Reserved.

struct Push_Constants 
{
  uint32_t        vertex_buffer_id;
  uint32_t        linear_sampler_id;
  uint32_t        anisotropic_sampler_id;
  uint32_t        camera_buffer_id;
  uint32_t        material_buffer_id;

  uint32_t        argument_buffer_id;
  uint32_t        argument_base_index;

  uint32_t        skinning_buffer_id;
};

// Per-instance drawing parameters.
struct Arguments 
{
  float4x4        transform;
  uint32_t        material_index;

  // Number of skinning matrices 
  // and the index into the buffer.
  uint32_t        num_matrices;
  uint32_t        skinning_matrices;
};
