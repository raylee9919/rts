// Copyright Seong Woo Lee. All Rights Reserved.

R"(

uniform mat4  world_transform;
uniform mat4  VP;
uniform int   is_skeletal;

layout (location = 0) in vec3 vP;
layout (location = 1) in vec3 vN;
layout (location = 2) in vec2 vUV;
layout (location = 3) in vec4 vC;
layout (location = 4) in vec3 v_tangent;

uniform mat4                    bone_transforms[MAX_BONE_PER_MESH];
layout (location = 5) in int    bone_ids[MAX_BONE_PER_VERTEX];
layout (location = 6) in float  bone_weights[MAX_BONE_PER_VERTEX];

uniform v2 uv_scale;

out smooth vec3 fP;
out smooth vec3 fN;
out smooth vec2 fUV;
out smooth vec4 fC;
out smooth mat3 TBN;

void main()
{
    mat4 M;
    if (is_skeletal != 0) {
        mat4 bone_transform;
        if (bone_ids[0] != -1) {
            bone_transform = bone_transforms[bone_ids[0]] * bone_weights[0];
            for (int idx = 1; idx < MAX_BONE_PER_VERTEX; ++idx) {
                int bone_id = bone_ids[idx];
                if (bone_id != -1) {
                    bone_transform += bone_transforms[bone_id] * bone_weights[idx];
                } else {
                    break;
                }
            }
        } else {
            bone_transform = identity();
        }
        M = world_transform * bone_transform;
    } else {
        M = world_transform;
    }


    fN = normalize(mat3(M) * vN);
    vec3 tangent = normalize(mat3(M) * v_tangent);
    vec3 bitangent = normalize(cross(fN, tangent));
    TBN = mat3(tangent, bitangent, fN);

    v4 result_pos = M * vec4(vP,1);
    result_pos /= result_pos.w;
    fP = result_pos.xyz;

    fUV = vUV * uv_scale;
    fC  = vC;

    gl_Position = VP * result_pos;
}

)";
