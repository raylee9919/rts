/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Seong Woo Lee $
   $Notice: (C) Copyright 2025 by Seong Woo Lee. All Rights Reserved. $
   ======================================================================== */

// #Todo: This value must be shared by graphics APIs.
//
#define MAX_LIGHTS              10
#define SHADOWMAP_RESOLUTION    1024
#define CSM_COUNT               4
static_assert(CSM_COUNT > 0, "CSM_COUNT must be bigger than 0!");


// # Note: Constants
//
#define RGBA_WHITE      v4{1.0f, 1.0f, 1.0f, 1.0f}
#define RGBA_BLACK      v4{0.0f, 0.0f, 0.0f, 1.0f}
#define RGBA_RED        v4{1.0f, 0.0f, 0.0f, 1.0f}
#define RGBA_GREEN      v4{0.0f, 1.0f, 0.0f, 1.0f}
#define RGBA_BLUE       v4{0.0f, 0.0f, 1.0f, 1.0f}
#define RGBA_MAGENTA    v4{1.0f, 0.0f, 1.0f, 1.0f}


struct Renderer;
struct Render_Commands;
struct Platform_Renderer;
struct Mesh;
struct Material;
struct Camera;

#define RENDERER_BEGIN_FRAME(NAME) Render_Commands *NAME(Platform_Renderer *renderer, v2u os_window_dim, v2u render_dim)
typedef RENDERER_BEGIN_FRAME(Renderer_Begin_Frame);

#define RENDERER_END_FRAME(NAME) void NAME(Platform_Renderer *platform_renderer, Renderer *renderer, Render_Commands *frame)
typedef RENDERER_END_FRAME(Renderer_End_Frame);

enum Render_Type 
{
    RENDER_TYPE_NULL = 0,

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
    v2u         window_dim;
    v2u         render_dim;
    
    u64         push_buffer_size;
    u64         push_buffer_used;
    u8         *push_buffer_base;

    Mesh       *sphere_mesh;

    v3          main_eye_position;
    m4x4        main_view_proj;
    m4x4        ortho_view_proj;

    b32         wireframe_mode;
    v4          wireframe_color;

    b32         draw_csm_frustum;
    b32         draw_csm_sphere;
    v3          csm_frustum_positions[8];
    v3          csm_to_light;
    m4x4        csm_view;
    b32         csm_varient_method;

    b32         draw_navmesh;

    b32         skybox_on;
    Mesh       *skybox_mesh;
    Bitmap     *skybox_textures[6];
    m4x4        skybox_eye_view_proj;

    v2          toggled_down_mouse_position;
    u32         toggled_down_entity_id;
    u32         hot_entity_id;
    u32         active_entity_id;

    m4x4        debug_transform;
    f32         debug_radius;
};

internal void
push_mesh(Render_Group *group, Mesh *mesh,
          m4x4 world_transform, m4x4 *animation_transforms, u32 entity_id, v2 uv_scale, v4 tint);

internal void
push_mesh(Render_Group *group, Mesh *mesh,
          m4x4 world_transform, m4x4 *animation_transforms, u32 entity_id, v2 uv_scale);

internal void
draw_line(Render_Group *group, v3 a, v3 b, v4 color);


// ----------------------------------------------------------
// # Todo: Revamping renderer currently..
//
struct Render_Id
{
    u64 e[1];
};

enum Render_Vertex_Type
{
    RENDER_VERTEX_TYPE_QUAD = 0,

    RENDER_VERTEX_TYPE_COUNT
};

struct Render_Vertex
{
    Render_Vertex_Type type;

    v2          position;
    v2          uv;
    Render_Id   texture_id;
    v4          color;
    v2          rect_center;
    v2          rect_half_dim;
    f32         radius;
};

typedef u16 Render_Texture_Type;
enum
{
    RENDER_TEXTURE_TYPE_INVALID  = 0,
    RENDER_TEXTURE_TYPE_R8G8B8A8 = 1,
};

struct Render_Buffer
{
    Render_Vertex  *vertices;
    u64             vertex_count;
    u64             instance_count;
};

struct Render_Texture
{
    Render_Texture      *first;
    Render_Texture      *last;
    Render_Texture      *next;
    Render_Texture      *prev;

    Render_Id           id;
    Render_Texture_Type type;
    void               *data;
    u32                 width;
    u32                 height;
};

typedef u32 Render_Command_Flags;
enum 
{
    RENDER_COMMAND_FLAG_TEXTURE_CREATE              = (1<<0),
    RENDER_COMMAND_FLAG_TEXTURE_DESTROY             = (1<<1),
    RENDER_COMMAND_FLAG_TEXTURE_FILTER_DOT          = (1<<2),
    RENDER_COMMAND_FLAG_TEXTURE_FILTER_LINEAR       = (1<<3),
    RENDER_COMMAND_FLAG_TEXTURE_WRAP                = (1<<4),
    RENDER_COMMAND_FLAG_TEXTURE_MIPMAP              = (1<<5),
};

struct Render_Command
{
    Render_Command_Flags flags;

    Render_Texture texture;
};

struct Renderer
{
    Arena *arena;

    // # Note: Vertex/Instance Buffer
    Render_Buffer buffer[RENDER_VERTEX_TYPE_COUNT];
    
    // # Note: Texture
    Arena           *texture_arena;
    Render_Texture  *texture_free_first;
    Render_Texture  *texture_free_last;
    Render_Texture  *texture_table;
    u64              texture_table_size;
    u64              texture_next_id;

    // # Note: Command Buffer
    Arena          *command_arena;
    Render_Command *commands;
    u64             command_count;
};

typedef u8 Render_String_Flags;
enum 
{
    RENDER_STRING_FLAG_NO_DRAW      = (1<<0),
    RENDER_STRING_FLAG_COMPUTE_SIZE = (1<<1),
    RENDER_STRING_FLAG_CULL         = (1<<2),
};


// # Note: Constants
//
#define render_max_vertex_count     16384
#define render_max_command_count    4096


// # Note: Texture.
//
#define render_texture_create_filter_dot(t,d,w,h) render_texture_create_flags(t,d,w,h,RENDER_COMMAND_FLAG_TEXTURE_FILTER_DOT)
#define render_texture_create_filter_linear(t,d,w,h) render_texture_create_flags(t,d,w,h,RENDER_COMMAND_FLAG_TEXTURE_FILTER_LINEAR)
internal Render_Id render_texture_create_flags(Render_Texture_Type type, void *data, u32 width, u32 height, Render_Command_Flags flags);
internal void render_texture_destroy(Render_Id id);



// # Note: Drawing Functions.
//
#define render_quad(mn,mx)                          render_quad_t(render_id_null(), mn, mx)
#define render_quad_t(t,mn,mx)                      render_quad_tuv(t, mn, mx, v2{0,0}, v2{1,1})
internal void render_quad_c(v2 min, v2 max, v4 color);
internal void render_quad_c4(v2 min, v2 max, v4 c00, v4 c10, v4 c01, v4 c11);
internal void render_quad_cr(v2 min, v2 max, v4 c, f32 r);
internal void render_quad_c4r(v2 min, v2 max, v4 c00, v4 c10, v4 c01, v4 c11, f32 r);
internal void render_quad_c4r4(v2 min, v2 max, v4 c00, v4 c10, v4 c01, v4 c11, f32 r00, f32 r10, f32 r01, f32 r11);
internal void render_quad_tuv(Render_Id texture_id, v2 min, v2 max, v2 uv_min, v2 uv_max);
internal void render_quad_tuvc(Render_Id texture_id, v2 min, v2 max, v2 uv_min, v2 uv_max, v4 c);
internal void render_quad_tuvc4(Render_Id texture_id, v2 min, v2 max, v2 uv_min, v2 uv_max, v4 c00, v4 c10, v4 c01, v4 c11);
internal void render_quad_tuvc4r4(Render_Id texture_id, v2 min, v2 max, v2 uv_min, v2 uv_max, v4 c00, v4 c10, v4 c01, v4 c11, f32 r00, f32 r10, f32 r01, f32 r11);



internal AABB2 render_string(Face *face, Render_Id atlas, v2 origin, Utf8 string, Render_String_Flags flags, AABB2 cull_aabb);
