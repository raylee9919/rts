// Copyright Seong Woo Lee. All Rights Reserved.

R"(

layout (location = 0) in vec3 vP;
layout (location = 5) in int    bone_ids[MAX_BONE_PER_VERTEX];
layout (location = 6) in float  bone_weights[MAX_BONE_PER_VERTEX];

uniform mat4  world_transform;
uniform int   is_skeletal;
uniform uint  index_to_my_skinning_matrices;

layout(std430, row_major, binding = 8) readonly buffer Skinning_Matrices
{
    mat4 skinning_matrices[];
};

out smooth vec3 fP;

void main()
{
    vec3 model_position = vec3(0);

    if (is_skeletal == 0) {
        model_position = vP;
    } else {
        f32 weights_sum = 0.0;
        for (int i = 0; i < MAX_BONE_PER_VERTEX && bone_ids[i] != -1; i += 1) {
            int bone_id = bone_ids[i];
            float weight = bone_weights[i];
            weights_sum += weight;

            mat4 skinning_matrix = skinning_matrices[index_to_my_skinning_matrices + bone_id];

            vec3 pose_position = (skinning_matrix * vec4(vP, 1)).xyz;
            model_position += pose_position * weight;
        }

        if (weights_sum > 1e-8) {
            float inv = 1.0 / weights_sum;
            model_position *= inv;
        }
    }

    gl_Position = world_transform * vec4(model_position, 1.0);
}

)";
