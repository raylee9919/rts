/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Seong Woo Lee $
   $Notice: (C) Copyright 2025 by Seong Woo Lee. All Rights Reserved. $
   ======================================================================== */

// @Todo: This value must be shared by graphics APIs.
#define MAX_LIGHTS              10
#define SHADOWMAP_RESOLUTION    1024
#define CSM_COUNT               4
static_assert(CSM_COUNT > 0, "CSM_COUNT must be bigger than 0!");


struct Render_Commands;
struct Platform_Renderer;
struct Mesh;
struct Material;
struct Camera;

#define RENDERER_BEGIN_FRAME(NAME) Render_Commands *NAME(Platform_Renderer *renderer, v2u os_window_dim, v2u render_dim)
typedef RENDERER_BEGIN_FRAME(Renderer_Begin_Frame);

#define RENDERER_END_FRAME(NAME) void NAME(Platform_Renderer *renderer, Render_Commands *frame)
typedef RENDERER_END_FRAME(Renderer_End_Frame);

enum Render_Type 
{
    eRender_Invalid = 0,

    eRender_Mesh,
    eRender_Bitmap,
    eRender_Triangles,
    eRender_Line,
};

struct Render_Entity_Header 
{
    Render_Type type;
    u64 size;
};

struct Render_Quad 
{
    Render_Entity_Header header;
    Bitmap *bitmap;
};

struct Render_Mesh 
{
    Render_Entity_Header header;
    Mesh *mesh;
    m4x4 world_transform;
    u32 entity_id;
    m4x4 *animation_transforms;
    v2 uv_scale;
    v4 tint;
};

struct Render_Triangles 
{
    Render_Entity_Header header;
    Vertex *vertices;
    u32 vertexcount;
    u32 *indices;
    u32 numtri;
    v4 color;
};

struct Render_Line 
{
    Render_Entity_Header header;
    v3 p[2];
    v4 color;
};

struct Render_Bitmap 
{
    Render_Entity_Header header;
    Bitmap *bitmap;
    v4 color;
    v3 min;
    v3 max;
};

struct Textured_Vertex 
{
    v3 pos;
    v2 uv;
};

struct Render_Group 
{
    u64 capacity;
    u64 used;
    u8 *base;
};

struct Platform_Renderer 
{
    void *platform;
};

struct Render_Commands 
{
    u32 version;
    f32 time;
    
    v2u window_dim;
    v2u render_dim;
    
    u64 push_buffer_size;
    u64 push_buffer_used;
    u8 *push_buffer_base;

    Mesh *sphere_mesh;

    Input input;

    v3 main_eye_position;
    m4x4 main_view_proj;
    m4x4 ortho_view_proj;

    b32 wireframe_mode;
    v4 wireframe_color;

    b32 draw_csm_frustum;
    b32 draw_csm_sphere;
    v3 csm_frustum_positions[8];
    v3 csm_to_light;
    m4x4 csm_view;
    b32 csm_varient_method;

    b32 draw_navmesh;

    b32 skybox_on;
    Mesh *skybox_mesh;
    Bitmap *skybox_textures[6];
    m4x4 skybox_eye_view_proj;

    v2 toggled_down_mouse_position;
    u32 toggled_down_entity_id;
    u32 hot_entity_id;
    u32 active_entity_id;

    m4x4 debug_transform;
    f32 debug_radius;
};
