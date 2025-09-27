#ifndef RTS_RENDERER_OPENGL_H
#define RTS_RENDERER_OPENGL_H
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Seong Woo Lee $
   $Notice: (C) Copyright 2025 by Seong Woo Lee. All Rights Reserved. $
   ======================================================================== */


#include <gl/gl.h>
#include "renderer/opengl/rts_gl_corearb.h"

// # Note: Coordinate
//
//         -NDC
//         ^
//         |
//     -------|------->


// # Note: Globals and Constants.
//
global const char *g_shader_header = 
#include "shader/header.glsl"
global char g_shared[2048];

#define gl_max_uniform_count    16
#define gl_max_attrib_count     16
// # Debug
#define gl_printf printf



struct Opengl_Info 
{
    b32 modern_context;

    char *vendor;
    char *renderer;
    char *version;
    char *shading_language_version;

    b32 opengl_ext_texture_sgb;
    b32 opengl_ext_framebuffer_srgb;
    b32 opengl_arb_framebuffer_object;
};

struct Sprite_Program 
{
    s32 id;

    s32 mvp;
    s32 color;
    s32 texture;
};

enum Pbr_Program_Flags 
{
    Pbr_Has_albedo    = (1 << 0),
    Pbr_Has_normal    = (1 << 1),
    Pbr_Has_roughness = (1 << 2),
    Pbr_Has_metalic   = (1 << 3),
    Pbr_Has_emission  = (1 << 4),
    Pbr_Has_orm       = (1 << 5),
    Pbr_No_Lighting   = (1 << 6),
};

struct Pbr_Program 
{
    s32 id;

    s32 world_transform;
    s32 VP;
    s32 is_skeletal;
    s32 bone_transforms;

    s32 uv_scale;

    s32 entity_id;
    s32 hot_entity_id;
    s32 active_entity_id;

    s32 wireframe_color;
    s32 tint;
    s32 eye_position;
    s32 csm_view;
    s32 flags;
    s32 shadowmap_view_projs;
    s32 to_light;
    s32 csm_z_spans;
};

struct Ground_Program 
{
    s32 id;

    s32 entity_id;
    s32 hot_entity_id;
    s32 active_entity_id;

    s32 model;
    s32 eye_position;
    s32 csm_view;
    s32 view_proj;
    s32 elevation;
    s32 flags;
    s32 wireframe_color;
    s32 shadowmap_view_projs;
    s32 to_light;
    s32 csm_z_spans;
};

struct Skybox_Program 
{
    s32 id;
    
    s32 view_proj;
};

struct Shadowmap_Program 
{
    s32 id;

    s32 world_transform;
    s32 VP;
    s32 is_skeletal;
    s32 bone_transforms;
    s32 light_view_projs;
};

struct Shadowmap_Ground_Program 
{
    s32 id;

    s32 model;
    s32 eye_position;
    s32 view_proj;
    s32 elevation;
    s32 light_view_projs;
};

struct Simple_Program 
{
    s32 id;

    s32 VP;
    s32 color;
};

struct Blt_Program 
{
    s32 id;
};

struct Circle_Program 
{
    s32 id;

    s32 model;
    s32 view_proj;
    s32 radius;
};

// # Note: Program (analogous to pipeline)
//
struct Gl_Vertex
{
};

struct Gl_Uniform
{
    GLuint id;
    Utf8 name;
};

struct Gl_Attrib
{
    GLint  location;
    GLenum type;
};

typedef u32 Gl_Program_Flags;
enum
{
    GL_PROGRAM_CULL_OFF = (1<<0),
};

struct Gl_Program
{
    GLuint id;

    Gl_Uniform  uniforms[gl_max_uniform_count];
    u32         uniform_count;

    Gl_Attrib   attribs[gl_max_attrib_count];
    u32         attrib_count;
};

// # Note: OpenGL
//
struct Opengl
{
    Platform_Renderer header;

    Opengl_Info info;

    GLint max_color_attachments;
    GLint max_samplers_per_shader;
    GLint max_multisample_count;
    b32 supports_srgb_framebuffer;

    Render_Commands render_commands;

    u8 *push_buffer;
    u64 push_buffer_size;

    GLuint  vao;
    GLuint  vbo;
    GLuint  vio;

    Sprite_Program          sprite_program;
    Pbr_Program             pbr_program;
    Ground_Program          ground_program;
    Skybox_Program          skybox_program;
    Shadowmap_Program       shadowmap_program;
    Shadowmap_Ground_Program shadowmap_ground_program;
    Simple_Program          simple_program;
    Blt_Program             blt_program;
    Circle_Program          circle_program;    

    GLuint fbo;
    GLuint depth_buffer;
    GLuint color_texture;
    GLuint id_texture;

    GLuint instance_vbo;

    GLuint shadowmap_fbo;
    GLuint shadowmaps;

    u32 skybox_texture;

    Bitmap  white_bitmap;
    u32     white[4][4];

    u32     fragment_counter;
    u32     alloc_count;
    u32     max_fragment_count;
    s32     max_compute_work_group_count[3];

    u32     octree_nodes;
    u32     octree_nodes_texture;
    u32     flist_P;
    u32     flist_P_texture;
    u32     flist_diffuse;
    u32     flist_diffuse_texture;

    u32     octree_diffuse;
    u32     octree_diffuse_texture;

    u32     entity_id_texture;



    // -----------------------------
    // # Note: Revamping
    
            

    Gl_Program quad_program;
};

// # Note: Macros.
//
#define GET_UNIFORM_LOCATION(Program, Name) gl->Program.Name = glGetUniformLocation(gl->Program.id, #Name);

// # Note: Function Declarations.
//
internal void opengl_debug_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam);
internal Opengl_Info opengl_get_info(Opengl *gl, b32 modern_context);
internal GLuint opengl_create_compute_program(Opengl *gl, const char *csrc);
internal GLuint opengl_create_program(Opengl *gl, const char *vsrc, const char *gsrc, const char *fsrc);
internal GLuint opengl_create_tessellation_program(Opengl *gl, const char *vs, const char *tcs, const char *tes, const char *fs);
internal GLuint opengl_create_tessellation_geometry_program(Opengl *gl, const char *vs, const char *tcs, const char *tes, const char *gs, const char *fs);
internal void opengl_alloc_texture(Opengl *gl, Bitmap *bitmap, GLenum wrapping, b32 generate_mipmap);
internal void opengl_bind_texture(Opengl *gl, Bitmap *bitmap, b32 generate_mipmap);
internal void opengl_bind_atomic_counter(Opengl *gl, s32 id, s32 binding_point);
internal void opengl_gen_linear_buffer(Opengl *gl, u32 *buf, u32 *tex, GLenum format, size_t size);
internal void opengl_set_flags_for_wireframe_mode(u32 *flags);
internal void gl_pbr_bind_texture_and_set_flags(Opengl *gl, Mesh *mesh, GLuint slot, GLenum wrapping, b32 mipmap, Pbr_Texture_Type type, u32 *flags);
internal void opengl_compile_shaders(Opengl *gl);
internal Render_Commands *opengl_frame_begin(Opengl *gl, v2u window_dim, v2u render_dim);
internal void opengl_frame_end(Opengl *gl, Render_Commands *frame);

// # Note: Program
//
internal GLuint opengl_program_create_vf(Opengl *gl, char *vsrc, char *fsrc);
internal Gl_Program opengl_program_vf(Opengl *gl, char *vs_src, char *fs_src);
internal void opengl_program_begin(Gl_Program program, Gl_Program_Flags flags);
internal void opengl_program_end(Gl_Program program, Gl_Program_Flags flags);
#define opengl_program_scope(program, flags) defer_loop(opengl_program_begin(program, flags), opengl_program_end(program, flags))

// # Note: Init
//
internal void opengl_init(Opengl *gl);

#endif // RTS_RENDERER_OPENGL_H
