// Copyright Seong Woo Lee. All Rights Reserved.

#pragma once

#pragma pack(push, 1)

// # Todo: Clean those mangled gibberish.
//
#define MAX_BONE_PER_VERTEX     4
#define MAX_BONE_PER_MESH       200

#define PATH_TO_DATA_FROM_BUILD         "../data/"

#define ASSET_MESH_FILE_FORMAT          ".smsh"
#define ASSET_MESH_DIRECTORY            "mesh/"
#define ASSET_ANIMATION_FILE_FORMAT     ".sanm"
#define ASSET_ANIMATION_DIRECTORY       "animation/"
#define ASSET_MAP_FILE_FORMAT           ".smap"
#define ASSET_MAP_DIRECTORY             "map/"


typedef s32 Joint_Id;


struct Asset_Version {
    u8 minor;
    u8 major;
    u8 patch;
};

struct Asset_Vertex {
    v3  pos;
    v3  normal;
    v2  uv;
    v4  color;
    v4  tangent;

    s32 node_ids[MAX_BONE_PER_VERTEX];
    f32 node_weights[MAX_BONE_PER_VERTEX];
};

struct Asset_Mesh {
    u32 length;
    u8 *name;

    u32 vertex_count;
    Asset_Vertex *vertices;

    u32 index_count;
    u32 *indices;
};

struct Asset_Model {
    u32 mesh_count;
    Asset_Mesh *meshes;
};

struct Asset_Animation_Node {
    s32 id; // index in a joint array in skeleton.

    v3         *translations;
    Quaternion *rotations;
    v3         *scales;
};

struct Asset_Animation {
    u32 length;
    u8 *name;

    u32 num_samples;
    f32 duration;

    u32 num_nodes;
    Asset_Animation_Node *nodes;
};

//struct Bitmap {
//    s32 bits_per_channel;
//    s32 channel_count;
//    s32 width;
//    s32 height;
//    s32 pitch;
//    u32 size;
//    void *memory;
//
//    u32 handle;
//};

enum Asset_ID 
{
    Asset_Invalid = 0,
};

enum Asset_State 
{
    Asset_State_Unloaded,
    Asset_State_Queued,
    Asset_State_Loaded
};

//
// Mesh
//
struct Vertex {
    v3 position;
    v3 normal;
    v2 uv;
    u32 color;
    v4 tangent; // w: sign

    s32 node_ids[MAX_BONE_PER_VERTEX];
    f32 node_weights[MAX_BONE_PER_VERTEX];
};

enum PBR_Texture_Type {
    PBR_ALBEDO,
    PBR_NORMAL,
    PBR_ROUGHNESS,
    PBR_METALLIC,
    PBR_EMISSION,

    PBR_COUNT,
};

struct Material {
    Asset::Texture textures[PBR_COUNT];
};

struct Mesh  
{
    String name;

    m4x4    global_transform;

    u32     num_vertices;
    Vertex* vertices;

    u32     num_indices;
    u32*    indices;

    Material material;
};

struct Node 
{
    s32 id;

    m4x4 offset;
    m4x4 base_transform;  // transform in parent's bone-space. aiNode
    m4x4 current_transform;

    u32 child_count;
    s32 *child_ids;
};

struct Model {
    u32 num_meshes;
    Mesh* meshes;
};


//
// Skeleton
//
struct Joint {
    String name;
    s32  parent;

    m4x4  local_transform;
    Xform local_xform;

    m4x4 inverse_bind_pose;
};

struct Skeleton {
    m4x4 root_transform; // not the root bone's transform.

    u32 num_joints;
    Joint *joints;
};


//
// Animation
//
struct Animation_Joint {
    s32 id;
    Xform *keyframes;
};

struct Animation_Joint_Entry {
    union {
        Animation_Joint_Entry *first;
        Animation_Joint_Entry *next;
    };
    Animation_Joint_Entry *last;
    Animation_Joint *joint;
};

struct Animation {
    String name;

    f32 duration;
    u32 num_keyframes;

    u32 num_joints;
    Animation_Joint *joints;

    
    // (s32 id) -> (Animation_Joint *joint) hash-table.
    Animation_Joint_Entry *joint_table;
    u32 table_size;
};





internal u32 get_triangle_count(Model *model);
internal u64 animation_hash(u32 id, u32 length);


internal void load_model(Arena *arena, Model *model_out, String file_path, v3 scale = v3{1.f,1.f,1.f});
internal void load_skeleton(Arena *arena, Skeleton *skel_out, String file_path);
internal void load_animation(Arena *arena, Animation *anim_out, String file_path);


#pragma pack(pop)
