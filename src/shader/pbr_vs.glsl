// Copyright Seong Woo Lee. All Rights Reserved.

R"(

layout (location = 0) in vec3 vP;
layout (location = 1) in vec3 vN;
layout (location = 2) in vec2 vUV;
layout (location = 3) in vec4 vC;
layout (location = 4) in vec4 v_tangent;
layout (location = 5) in int    bone_ids[MAX_BONE_PER_VERTEX];
layout (location = 6) in float  bone_weights[MAX_BONE_PER_VERTEX];

uniform mat4 u_view_proj;

out VS_Out 
{
    vec3 fP;
    vec3 fN;
    vec3 fT;
    vec2 fUV;
    vec4 fC;
    float fSign;
    flat int draw_id;
} vs_out;

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

void main()
{
    vec3 model_position = vec3(0);
    vec3 model_normal   = vec3(0);
    vec3 model_tangent  = vec3(0);

    Geometry_Param geo = geometry_params[gl_DrawID];

    if (geo.is_skeletal == 0) {
        model_position = vP;
        model_normal   = vN;
        model_tangent  = v_tangent.xyz;
    } else {
        f32 weights_sum = 0.0;
        for (int i = 0; i < MAX_BONE_PER_VERTEX && bone_ids[i] != -1; i += 1) {
            int bone_id = bone_ids[i];
            float weight = bone_weights[i];
            weights_sum += weight;

            mat4 skinning_matrix = skinning_matrices[geo.index_to_my_skinning_matrices + bone_id];

            vec3 pose_position = (skinning_matrix * vec4(vP, 1)).xyz;
            model_position += pose_position * weight;

            vec3 pose_normal = (skinning_matrix * vec4(vN, 0)).xyz;
            model_normal += pose_normal * weight;

            vec3 pose_tangent = (skinning_matrix * vec4(v_tangent.xyz, 0)).xyz;
            model_tangent += pose_tangent * weight;
        }

        if (weights_sum > 1e-8) {
            float inv = 1.0 / weights_sum;
            model_position *= inv;
        }
    }

    mat4 world_matrix = geo.world_transform;
    model_position = (world_matrix * vec4(model_position, 1)).xyz;
    model_normal = (world_matrix * vec4(model_normal, 0)).xyz;
    model_tangent = (world_matrix * vec4(model_tangent, 0)).xyz;

    vs_out.fN = normalize(model_normal);
    vs_out.fT = normalize(model_tangent);
    vs_out.fT = normalize(vs_out.fT - dot(vs_out.fT, vs_out.fN) * vs_out.fN); // Gram-Schmidt
    vs_out.fSign = v_tangent.w;

    vs_out.fP  = model_position;
    vs_out.fUV = vUV * geo.uv_scale;
    vs_out.fC  = vC;

    vs_out.draw_id = gl_DrawID;

    gl_Position = u_view_proj * vec4(model_position, 1);
}

)";
