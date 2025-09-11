/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Sung Woo Lee $
   $Notice: (C) Copyright %s by Sung Woo Lee. All Rights Reserved. $
   ======================================================================== */

#include "asset_shared.h"

#pragma pack(push, 1)

struct Asset_Vertex {
    v3  pos;
    v3  normal;
    v2  uv;
    v4  color;
    v3  tangent;

    s32 node_ids[MAX_BONE_PER_VERTEX];
    f32 node_weights[MAX_BONE_PER_VERTEX];
};

struct Asset_Texture {
    s32 bits_per_channel;
    s32 channel_count;
    s32 width;
    s32 height;
    s32 pitch;
    u32 size;
    void *data;
};

struct Asset_Mesh {
    u32 vertex_count;
    Asset_Vertex *vertices;

    u32 index_count;
    u32 *indices;

#if 0
    u8 has_albedo;
    Asset_Texture *albedo;

    u8 has_normal;
    Asset_Texture *normal;

    u8 has_metalic;
    Asset_Texture *metalic;

    u8 has_roughness;
    Asset_Texture *roughness;

    u8 has_emission;
    Asset_Texture *emission;

    u8 has_ao;
    Asset_Texture *ao;
#endif
};

struct Asset_Material {
    v3 color_ambient;
    v3 color_diffuse;
    v3 color_specular;
};

struct Asset_Node {
    s32 id;

    m4x4 offset;     // mesh-space to bone-space. aiBone... if unavailable, set to no-op matrix...?
    m4x4 transform;  // transform in parent's bone-space. aiNode

    u32 child_count;
    s32 *child_ids;
};

struct Asset_Model {
    u32 mesh_count;
    Asset_Mesh *meshes;

    u32 material_count;
    Asset_Material *materials;

    u32 node_count;
    s32 root_bone_node_id;
    Asset_Node *nodes;
};


#pragma pack(pop)
