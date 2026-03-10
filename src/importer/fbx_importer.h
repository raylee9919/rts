// Copyright Seong Woo Lee. All Rights Reserved.

struct FBXI_Bone {
    u8 *name;
    u32 length;

    s32 parent;
    m4x4 local_transform;
    m4x4 inverse_bind_pose;
};

struct State {
    Arena *arena;

    Arena *scene_arena;
    ufbx_scene *scene;

    SMikkTSpaceContext mikkt_ctx;

    Asset_Mesh *meshes;
    u32 num_meshes;

    m4x4 root_transform;
    FBXI_Bone *bones;
    u32 num_bones;

    std::unordered_map <std::string, s32> bone_map; // name to id

    Asset_Animation *anims;
    int num_anims;

    m4x4 undo_pre_root_bone_transforms;
};
