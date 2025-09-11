R"(

uniform m4x4  world_transform;
uniform m4x4  VP;
uniform s32   is_skeletal;

layout (location = 0) in v3 vP;

out smooth v3 fP;

uniform m4x4                  bone_transforms[MAX_BONE_PER_MESH];
layout (location = 5) in s32  bone_ids[MAX_BONE_PER_VERTEX];
layout (location = 6) in f32  bone_weights[MAX_BONE_PER_VERTEX];

void main()
{
    m4x4 M;
    if (is_skeletal != 0) {
        m4x4 bone_transform;
        if (bone_ids[0] != -1) {
            bone_transform = bone_transforms[bone_ids[0]] * bone_weights[0];
            for (s32 idx = 1; idx < MAX_BONE_PER_VERTEX; ++idx) {
                s32 bone_id = bone_ids[idx];
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

    v4 result_pos = M * v4(vP,1);
    result_pos /= result_pos.w;
    fP = result_pos.xyz;

    gl_Position = VP * result_pos;
}

)";
