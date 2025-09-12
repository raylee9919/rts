#ifndef RTS_ASSET_H
#define RTS_ASSET_H
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Seong Woo Lee $
   $Notice: (C) Copyright %s by Seong Woo Lee. All Rights Reserved. $
   ======================================================================== */
#pragma pack(push, 1)

#define MAX_BONE_PER_VERTEX     4
#define MAX_BONE_PER_MESH       200
#define PATH_TO_DATA_FROM_BUILD         "../data/"
#define ASSET_MESH_FILE_FORMAT          ".smsh"
#define ASSET_MESH_DIRECTORY            "mesh/"
#define ASSET_ANIMATION_FILE_FORMAT     ".sanm"
#define ASSET_ANIMATION_DIRECTORY       "animation/"
#define ASSET_MAP_FILE_FORMAT           ".smap"
#define ASSET_MAP_DIRECTORY             "map/"

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

struct dt_v3_Pair {
    f32 dt;
    v3 vec;
};

struct dt_qt_Pair {
    f32 dt;
    Quaternion q;
};

struct Asset_Animation_Node {
    s32 id;

    u32 translation_count;
    u32 rotation_count;
    u32 scaling_count;

    dt_v3_Pair  *translations;
    dt_qt_Pair  *rotations;
    dt_v3_Pair  *scalings;
};

struct Asset_Animation {
    char *name;

    f32  duration;
    u32  node_count;

    Asset_Animation_Node *nodes;
};

// ------------------------------------
// @Note: 
struct Bitmap 
{
    s32 bits_per_channel;
    s32 channel_count;
    s32 width;
    s32 height;
    s32 pitch;
    u32 handle;
    u32 size;
    void *memory;
};

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

// --------------------------------
// @Note: Model
struct Vertex 
{
    v3 position;
    v3 normal;
    v2 uv;
    v4 color;
    v3 tangent;

    s32 node_ids[MAX_BONE_PER_VERTEX];
    f32 node_weights[MAX_BONE_PER_VERTEX];
};

enum Pbr_Texture_Type 
{
    Pbr_Texture_Albedo,
    Pbr_Texture_Normal,
    Pbr_Texture_Metalic,
    Pbr_Texture_Roughness,
    Pbr_Texture_Emission,
    Pbr_Texture_Orm,

    Pbr_Texture_Count,
};

struct Mesh 
{
    u32 vertex_count;
    Vertex *vertices;

    u32 index_count;
    u32 *indices;

    Bitmap textures[Pbr_Texture_Count];
};

struct Material 
{
    v3 color_ambient;
    v3 color_diffuse;
    v3 color_specular;
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

struct Model 
{
    u32 mesh_count;
    Mesh *meshes;

    u32 material_count;
    Material *materials;

    u32 node_count;
    s32 root_bone_node_id;
    Node *nodes;
};

// ------------------------------------
// @Note: Animation
struct Animation_Hash_Slot 
{
    u32 id;
    u32 idx;
    Animation_Hash_Slot *next;
};

struct Animation_Hash_Entry 
{
    Animation_Hash_Slot *first;
};

struct Animation_Hash_Table 
{
    u32 entry_count;
    Animation_Hash_Entry *entries;
};

struct Sample 
{
    s32         id;

    u32         translation_count;
    u32         rotation_count;
    u32         scaling_count;

    dt_v3_Pair  *translations;
    dt_qt_Pair  *rotations;
    dt_v3_Pair  *scalings;
};

struct Animation 
{
    char *name;

    f32 duration;

    u32 sample_count;
    Sample *samples;

    Animation_Hash_Table hash_table;
};

struct Animation_Channel 
{
    Animation *animation;
    f32 dt;
};

struct Node_Hash_Result 
{
    b32 found;
    u32 idx;
};

struct TRS 
{
    v3 translation;
    Quaternion rotation;
    v3 scaling;
};

struct Eval_Stack_Frame
{
    s32 node_id;
    b32 global_transform_done;
    u32 next_child_idx;

    m4x4 global_transform;
};

struct Eval_Stack
{
    Eval_Stack_Frame frames[256];
    u32 top;
};
    

// --------------------------------
// @Note: Font
struct Kerning 
{
    u32         first;
    u32         second;
    s32         value;
    Kerning     *prev;
    Kerning     *next;
};

struct Kerning_List 
{
    Kerning     *first;
    Kerning     *last;
    u32         count;
};

struct Kerning_Hashmap 
{
    Kerning_List entries[1024];
};

struct Asset_Font_Header 
{
    u32 kerning_pair_count;
    u32 vertical_advance;
    f32 ascent;
    f32 descent;
    f32 max_width;
};

struct Asset_Kerning 
{
    u32 first;
    u32 second;
    s32 value;
};

struct Asset_Glyph 
{
    u32 codepoint;
    s32 ascent;
    s32 A;
    s32 B;
    s32 C;
    Bitmap bitmap;
};

struct Asset_Font 
{
    u32             v_advance;
    f32             ascent;
    f32             descent;
    f32             max_width;
    Kerning_Hashmap kern_hashmap;
    Asset_Glyph     *glyphs[256];
    Kerning kernings[4096];

    // @Temporary
    Buffer buffer;
    u64 writetime;
};


internal void load_font(Arena *arena, char *filepath, Asset_Font *font);
internal void load_image(Bitmap *bitmap, char *filepath, Arena *arena);
internal void load_model(Model *model, const char *filename, Arena *arena);
internal u32 get_triangle_count(Model *model);
internal u32 animation_hash(u32 id, u32 length);
internal void load_animation(Animation *anim, const char *filename, Arena *arena);


// -------------------------------------
// @Note: Animation.
internal Node_Hash_Result get_sample_index(Animation *anim, u32 id);
internal void accumulate(Animation_Channel *channel, f32 dt);
internal TRS interpolate_trs(TRS trs1, f32 t, TRS trs2);
internal TRS interpolate_sample(Sample *sample, f32 dt);
internal void eval_node(Animation *anim, f32 dt, Node *node);
internal void eval(Model *model, Animation *anim, f32 dt, m4x4 *final_transforms, b32 do_eval_node);
internal void interpolate(Model *model, Animation *anim1, f32 dt1, f32 t, Animation *anim2, f32 dt2);

// -------------------------------------
// @Note: Font.
internal u32 kerning_hash(Kerning_Hashmap *hashmap, u32 first, u32 second);
internal void push_kerning(Kerning_Hashmap *hashmap, Kerning *kern, u32 entry_idx);
internal s32 get_kerning(Kerning_Hashmap *hashmap, u32 first, u32 second);



#pragma pack(pop)

#endif // RTS_ASSET_H
