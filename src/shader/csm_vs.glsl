// Copyright Seong Woo Lee. All Rights Reserved.

R"(

layout (location = 0) in vec3   vP;
layout (location = 5) in int    bone_ids[MAX_BONE_PER_VERTEX];
layout (location = 6) in float  bone_weights[MAX_BONE_PER_VERTEX];

struct Geometry_Param
{
    mat4  world_transform;
    int   is_skeletal;
    uint  index_to_my_skinning_matrices;
    vec2  uv_scale;
};

layout(std430, row_major, binding = 10) readonly buffer Geometry_SSBO
{
    Geometry_Param geometry_params[];
};

layout(std430, row_major, binding = 8) readonly buffer Skinning_Matrices
{
    mat4 skinning_matrices[];
};

uniform m4x4 light_view_projs[CSM_COUNT];


void main()
{
    vec3 model_position = vec3(0);

    Geometry_Param geo = geometry_params[gl_DrawID];

    if (geo.is_skeletal == 0) {
        model_position = vP;
    } else {
        f32 weights_sum = 0.0;
        for (int i = 0; i < MAX_BONE_PER_VERTEX && bone_ids[i] != -1; i += 1) {
            int bone_id = bone_ids[i];
            float weight = bone_weights[i];
            weights_sum += weight;

            mat4 skinning_matrix = skinning_matrices[geo.index_to_my_skinning_matrices + bone_id];

            vec3 pose_position = (skinning_matrix * vec4(vP, 1)).xyz;
            model_position += pose_position * weight;
        }

        if (weights_sum > 1e-8) {
            float inv = 1.0 / weights_sum;
            model_position *= inv;
        }
    }

    gl_Position = light_view_projs[gl_InstanceID] * geo.world_transform * vec4(model_position, 1.0);
    gl_Layer = gl_InstanceID;
}

)";
