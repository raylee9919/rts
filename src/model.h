/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Sung Woo Lee $
   $Notice: (C) Copyright 2024 by Sung Woo Lee. All Rights Reserved. $
   ======================================================================== */

#include "asset_shared.h"

struct Bitmap;

struct Vertex {
    v3 position;
    v3 normal;
    v2 uv;
    v4 color;
    v3 tangent;

    s32 node_ids[MAX_BONE_PER_VERTEX];
    f32 node_weights[MAX_BONE_PER_VERTEX];
};

enum Pbr_Texture_Type {
    Pbr_Texture_Albedo,
    Pbr_Texture_Normal,
    Pbr_Texture_Metalic,
    Pbr_Texture_Roughness,
    Pbr_Texture_Emission,
    Pbr_Texture_Orm,

    Pbr_Texture_Count,
};

struct Mesh {
    u32 vertex_count;
    Vertex *vertices;

    u32 index_count;
    u32 *indices;

    Bitmap textures[Pbr_Texture_Count];
};

struct Material {
    v3 color_ambient;
    v3 color_diffuse;
    v3 color_specular;
};

struct Node {
    s32 id;

    m4x4 offset;
    m4x4 base_transform;  // transform in parent's bone-space. aiNode
    m4x4 current_transform;

    u32 child_count;
    s32 *child_ids;
};

struct Model {
    u32 mesh_count;
    Mesh *meshes;

    u32 material_count;
    Material *materials;

    u32 node_count;
    s32 root_bone_node_id;
    Node *nodes;
};
