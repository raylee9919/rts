// Copyright Seong Woo Lee. All Rights Reserved.

#pragma once


#include <gl/gl.h>
#include "third_party/opengl/glcorearb.h"

#include "./gl_x.h"
#include "./gl_bump_allocator.h"
#include "./gl_texture.h"
#include "./gl_resource.h"


// Globals and Constants.
//
global const char *g_shader_header = 
#include "shader/header.glsl"
global char g_shared[2048];

#define gl_max_uniform_count    16
#define gl_max_attrib_count     16



struct GL_Info
{
    b32 modern_context;

    char *vendor;
    char *renderer;
    char *version;
    char *shading_language_version;

    // Constants
    GLint max_texture_units;
    GLint max_attachment;

    // ARB/EXT
    bool ARB_framebuffer_object;
    bool EXT_texture_srgb;
    bool ARB_EXT_framebuffer_srgb;
    bool ARB_direct_state_access;
    bool ARB_explicit_uniform_location;
    bool ARB_bindless_texture;
};

struct Sprite_Program 
{
    s32 id;

    s32 mvp;
    s32 color;
    s32 texture;
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

    s32 tint;

    GLuint ubo;
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

struct GL_Mesh_Buffer {
    // @Robustness
    Mesh*  mesh;

    // Suballocated in a big mesh buffer.
    u64 vertex_offset; // offset to vertices.
    u64 index_offset;  // offset to indices.

    GL_Mesh_Buffer* next;
};


struct Opengl
{
    Platform_Renderer header;

    GL_Info info;

    Render_Commands render_commands;

    u32 last_draw_width;
    u32 last_draw_height;

    u8 *push_buffer;
    u64 push_buffer_size;

    GLuint  vao;
    GLuint  vbo;
    GLuint  vio;

    Pbr_Program              pbr_program;
    Sprite_Program           sprite_program;
    Skybox_Program           skybox_program;
    Shadowmap_Program        shadowmap_program;
    Simple_Program           simple_program;
    Blt_Program              blt_program;


    GLuint fbo;
    GLuint color_texture;
    GLuint depth_texture;

    GLuint shadowmap_fbo;
    GLuint shadowmaps;

    u32 skybox_texture;


    // Default bound textures.
    //
    GLuint white_texture;
    u32    white[4][4];

    GLuint black_texture;
    u32    black[4][4];

    GLuint blue_texture;
    u32    blue[4][4];


    GL::Bump_Allocator vertex_buffer;
    GL::Bump_Allocator index_buffer;



    // VBO list.
    GL_Mesh_Buffer *first_mesh_buffer;
    GL_Mesh_Buffer *last_mesh_buffer;


    
    Gl_Program quad_program;


    GLuint texture_buffer;
    u32 max_num_textures;
    u32 num_textures;
    GL::Texture* first_texture;
    GL::Texture* last_texture;
};

#define GET_UNIFORM_LOCATION(Program, Name) gl->Program.Name = glGetUniformLocation(gl->Program.id, #Name);

// Function Declarations.
//
internal void opengl_debug_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam);
internal GL_Info opengl_get_info(Opengl *gl, b32 modern_context);
internal GLuint opengl_create_compute_program(Opengl *gl, const char *csrc);
internal GLuint opengl_create_program(Opengl *gl, const char *vsrc, const char *gsrc, const char *fsrc);
internal GLuint opengl_create_tessellation_program(Opengl *gl, const char *vs, const char *tcs, const char *tes, const char *fs);
internal GLuint opengl_create_tessellation_geometry_program(Opengl *gl, const char *vs, const char *tcs, const char *tes, const char *gs, const char *fs);
internal void opengl_set_flags_for_wireframe_mode(u32 *flags);
internal void opengl_compile_shaders(Opengl *gl);
internal Render_Commands *opengl_frame_begin(Opengl *gl, v2u window_dim, v2u render_dim);
internal void gl_frame_end(Opengl *gl, Render_Commands *frame);
internal void gl_bind_pbr_texture(Opengl *gl, Mesh *mesh, Pbr_Texture_Type type, int slot, GLuint default_handle);

internal GLuint opengl_program_create_vf(Opengl *gl, char *vsrc, char *fsrc);
internal Gl_Program opengl_program_vf(Opengl *gl, char *vs_src, char *fs_src);
internal void opengl_program_begin(Gl_Program program, Gl_Program_Flags flags);
internal void opengl_program_end(Gl_Program program, Gl_Program_Flags flags);
#define opengl_program_scope(program, flags) defer_loop(opengl_program_begin(program, flags), opengl_program_end(program, flags))

internal void gl_init(Opengl* gl);
