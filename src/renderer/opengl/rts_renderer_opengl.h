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


// ---------------------------------------------------------------
// @Note: Scrapped from OpenGL core header file.
typedef char    GLchar;
typedef size_t  GLsizeiptr;
typedef size_t  GLintptr;
typedef void (APIENTRY  *GLDEBUGPROCARB)(GLenum source,GLenum type,GLuint id,GLenum severity,GLsizei length,const GLchar *message,const void *userParam);
#define GL_SRGB8_ALPHA8                     0x8C43
#define GL_FRAMEBUFFER_SRGB                 0x8DB9            
#define GL_SHADING_LANGUAGE_VERSION         0x8B8C
#define GL_VERTEX_SHADER                    0x8B31
#define GL_GEOMETRY_SHADER                  0x8DD9
#define GL_FRAGMENT_SHADER                  0x8B30
#define GL_COMPUTE_SHADER                   0x91B9
#define GL_VALIDATE_STATUS                  0x8B83
#define GL_COMPILE_STATUS                   0x8B81
#define GL_LINK_STATUS                      0x8B82
#define GL_ARRAY_BUFFER                     0x8892
#define GL_ELEMENT_ARRAY_BUFFER             0x8893
#define GL_NUM_EXTENSIONS                   0x821D
#define GL_MAJOR_VERSION                    0x821B
#define GL_MINOR_VERSION                    0x821C
#define GL_CLAMP_TO_EDGE                    0x812F
#define GL_CLAMP_TO_BORDER                  0x812D
#define GL_STREAM_DRAW                      0x88E0
#define GL_STREAM_READ                      0x88E1
#define GL_STREAM_COPY                      0x88E2
#define GL_STATIC_DRAW                      0x88E4
#define GL_STATIC_READ                      0x88E5
#define GL_STATIC_COPY                      0x88E6
#define GL_DYNAMIC_DRAW                     0x88E8
#define GL_DYNAMIC_READ                     0x88E9
#define GL_DYNAMIC_COPY                     0x88EA
#define GL_DEBUG_OUTPUT                     0x92E0
#define GL_DEBUG_OUTPUT_SYNCHRONOUS         0x8242
#define GL_DEBUG_SEVERITY_HIGH              0x9146
#define GL_DEBUG_SEVERITY_MEDIUM            0x9147
#define GL_DEBUG_SEVERITY_LOW               0x9148
#define GL_CULL_FACE                        0x0B44
#define GL_NONE                             0
#define GL_FRONT_LEFT                       0x0400
#define GL_FRONT_RIGHT                      0x0401
#define GL_BACK_LEFT                        0x0402
#define GL_BACK_RIGHT                       0x0403
#define GL_FRONT                            0x0404
#define GL_BACK                             0x0405
#define GL_LEFT                             0x0406
#define GL_RIGHT                            0x0407
#define GL_FRONT_AND_BACK                   0x0408
#define GL_CW                               0x0900
#define GL_CCW                              0x0901
#define GL_BYTE                             0x1400
#define GL_UNSIGNED_BYTE                    0x1401
#define GL_SHORT                            0x1402
#define GL_UNSIGNED_SHORT                   0x1403
#define GL_INT                              0x1404
#define GL_R16                              0x822A
#define GL_UNSIGNED_INT                     0x1405
#define GL_FLOAT                            0x1406
#define GL_MULTISAMPLE                      0x809D
#define GL_SAMPLE_ALPHA_TO_COVERAGE         0x809E
#define GL_SAMPLE_ALPHA_TO_ONE              0x809F
#define GL_SAMPLE_COVERAGE                  0x80A0
#define GL_SAMPLE_BUFFERS                   0x80A8
#define GL_SAMPLES                          0x80A9
#define GL_SAMPLE_COVERAGE_VALUE            0x80AA
#define GL_SAMPLE_COVERAGE_INVERT           0x80AB
#define GL_MAX_SAMPLES                      0x8D57
#define GL_FRAMEBUFFER                      0x8D40
#define GL_DEPTH_ATTACHMENT                 0x8D00
#define GL_TEXTURE_3D                       0x806F
#define GL_TEXTURE_WRAP_R                   0x8072
#define GL_READ_ONLY                        0x88B8
#define GL_WRITE_ONLY                       0x88B9
#define GL_READ_WRITE                       0x88BA
#define GL_R8UI                             0x8232
#define GL_RGBA8UI                          0x8D7C
#define GL_RGBA_INTEGER                     0x8D99
#define GL_R32UI                            0x8236
#define GL_RGB32UI                          0x8D71
#define GL_RED_INTEGER                      0x8D94
#define GL_RGB_INTEGER                      0x8D98
#define GL_COLOR_ATTACHMENT0                0x8CE0
#define GL_COLOR_ATTACHMENT1                0x8CE1
#define GL_COLOR_ATTACHMENT2                0x8CE2
#define GL_COLOR_ATTACHMENT3                0x8CE3
#define GL_RGBA16F                          0x881A
#define GL_RGBA32UI                         0x8D70
#define GL_MAX_COLOR_ATTACHMENTS            0x8CDF
#define GL_TEXTURE0                         0x84C0
#define GL_TEXTURE1                         0x84C1
#define GL_TEXTURE2                         0x84C2
#define GL_TEXTURE3                         0x84C3
#define GL_RENDERBUFFER                     0x8D41
#define GL_ATOMIC_COUNTER_BUFFER            0x92C0
#define GL_DYNAMIC_STORAGE_BIT              0x0100
#define GL_MAP_READ_BIT                     0x0001
#define GL_MAP_WRITE_BIT                    0x0002
#define GL_SHADER_STORAGE_BUFFER            0x90D2
#define GL_UNSIGNED_INT_10_10_10_2          0x8036
#define GL_RGB10_A2UI                       0x906F
#define GL_RGBA8                            0x8058
#define GL_TEXTURE_BUFFER                   0x8C2A
#define GL_MAX_COMPUTE_WORK_GROUP_COUNT     0x91BE
#define GL_MAX_COMPUTE_WORK_GROUP_SIZE      0x91BF
#define GL_MAP_PERSISTENT_BIT               0x0040
#define GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT  0x00000001
#define GL_ELEMENT_ARRAY_BARRIER_BIT        0x00000002
#define GL_UNIFORM_BARRIER_BIT              0x00000004
#define GL_TEXTURE_FETCH_BARRIER_BIT        0x00000008
#define GL_SHADER_IMAGE_ACCESS_BARRIER_BIT  0x00000020
#define GL_COMMAND_BARRIER_BIT              0x00000040
#define GL_PIXEL_BUFFER_BARRIER_BIT         0x00000080
#define GL_TEXTURE_UPDATE_BARRIER_BIT       0x00000100
#define GL_BUFFER_UPDATE_BARRIER_BIT        0x00000200
#define GL_FRAMEBUFFER_BARRIER_BIT          0x00000400
#define GL_TRANSFORM_FEEDBACK_BARRIER_BIT   0x00000800
#define GL_ATOMIC_COUNTER_BARRIER_BIT       0x00001000
#define GL_ALL_BARRIER_BITS                 0xFFFFFFFF
#define GL_R8                               0x8229
#define GL_RG8                              0x822B
#define GL_RG                               0x8227
#define GL_RG16                             0x822C
#define GL_PATCH_VERTICES                   0x8E72
#define GL_TESS_EVALUATION_SHADER           0x8E87
#define GL_TESS_CONTROL_SHADER              0x8E88
#define GL_PATCHES                          0x000E
#define GL_TEXTURE_CUBE_MAP                 0x8513
#define GL_TEXTURE_BINDING_CUBE_MAP         0x8514
#define GL_TEXTURE_CUBE_MAP_POSITIVE_X      0x8515
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_X      0x8516
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Y      0x8517
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Y      0x8518
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Z      0x8519
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Z      0x851A
#define GL_DEPTH_COMPONENT16                0x81A5
#define GL_TEXTURE_2D_ARRAY                 0x8C1A
#define GL_DEPTH_COMPONENT32F               0x8CAC
#define GL_FRAMEBUFFER_COMPLETE             0x8CD5
#define GL_UNSIGNED_INT                     0x1405


// ---------------------------------------------------------------
// @Note: Scrapped from OpenGL core header file.
typedef BOOL        Type_wglSwapIntervalEXT(int interval);
typedef GLuint      Type_glCreateShader(GLenum shaderType);
typedef void        Type_glShaderSource(GLuint shader, GLsizei count, const GLchar **string, const GLint *length);
typedef void        Type_glCompileShader(GLuint shader);
typedef GLuint      Type_glCreateProgram(void);
typedef void        Type_glAttachShader(GLuint program, GLuint shader);
typedef void        Type_glLinkProgram(GLuint program);
typedef void        Type_glGetProgramiv(GLuint program, GLenum pname, GLint *params);
typedef void        Type_glGetShaderInfoLog(GLuint shader, GLsizei maxLength, GLsizei *length, GLchar *infoLog);
typedef void        Type_glValidateProgram(GLuint program);
typedef void        Type_glGetProgramInfoLog(GLuint program, GLsizei maxLength, GLsizei *length, GLchar *infoLog);
typedef void        Type_glGenBuffers(GLsizei n, GLuint *buffers);
typedef void        Type_glBindBuffer(GLenum target, GLuint buffer);
typedef void        Type_glUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
typedef GLint       Type_glGetUniformLocation(GLuint program, const GLchar *name);
typedef void        Type_glUseProgram(GLuint program);
typedef void        Type_glUniform1i (GLint location, GLint v0);
typedef void        Type_glBufferData(GLenum target, GLsizeiptr size, const void *data, GLenum usage);
typedef void        Type_glVertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer);
typedef GLint       Type_glGetAttribLocation (GLuint program, const GLchar *name);
typedef void        Type_glEnableVertexAttribArray (GLuint index);
typedef void        Type_glGenVertexArrays (GLsizei n, GLuint *arrays);
typedef void        Type_glBindVertexArray (GLuint array);
typedef void        Type_glBindAttribLocation (GLuint program, GLuint index, const GLchar *name);
typedef void        Type_glDebugMessageCallbackARB (GLDEBUGPROCARB callback, const void *userParam);
typedef void        Type_glDisableVertexAttribArray (GLuint index);
typedef void        Type_glUniform3fv (GLint location, GLsizei count, const GLfloat *value);
typedef void        Type_glVertexAttribIPointer (GLuint index, GLint size, GLenum type, GLsizei stride, const void *pointer);
typedef void        Type_glUniform4fv (GLint location, GLsizei count, const GLfloat *value);
typedef void        Type_glVertexAttribDivisor (GLuint index, GLuint divisor);
typedef void        Type_glDrawElementsInstanced (GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount);
typedef void        Type_glUniform1f (GLint location, GLfloat v0);
typedef void        Type_glUniform1fv (GLint location, GLsizei count, const GLfloat *value);
typedef void        Type_glGenFramebuffers (GLsizei n, GLuint *framebuffers);
typedef void        Type_glBindFramebuffer (GLenum target, GLuint framebuffer);
typedef void        Type_glFramebufferTexture2D (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
typedef void        Type_glTexStorage3D (GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth);
typedef void        Type_glTexSubImage3D (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void *pixels);
typedef void        Type_glGenerateMipmap (GLenum target);
typedef void        Type_glBindImageTexture (GLuint unit, GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum access, GLenum format);
typedef void        Type_glClearTexImage (GLuint texture, GLint level, GLenum format, GLenum type, const void *data);
typedef void        Type_glDrawBuffers (GLsizei n, const GLenum *bufs);
typedef void        Type_glActiveTexture (GLenum texture);
typedef void        Type_glBindRenderbuffer (GLenum target, GLuint renderbuffer);
typedef void        Type_glRenderbufferStorage (GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
typedef void        Type_glFramebufferRenderbuffer (GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
typedef void        Type_glGenRenderbuffers (GLsizei n, GLuint *renderbuffers);
typedef void        Type_glBufferSubData (GLenum target, GLintptr offset, GLsizeiptr size, const void *data);
typedef void        Type_glBufferStorage (GLenum target, GLsizeiptr size, const void *data, GLbitfield flags);
typedef void        Type_glBindBufferBase (GLenum target, GLuint index, GLuint buffer);
typedef void        Type_glGetBufferSubData (GLenum target, GLintptr offset, GLsizeiptr size, void *data);
typedef void        Type_glTexBuffer (GLenum target, GLenum internalformat, GLuint buffer);
typedef void        Type_glUniform1ui (GLint location, GLuint v0);
typedef void        Type_glDispatchCompute (GLuint num_groups_x, GLuint num_groups_y, GLuint num_groups_z);
typedef void        Type_glMemoryBarrier (GLbitfield barriers);
typedef void *      Type_glMapBufferRange (GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access);
typedef GLboolean   Type_glUnmapBuffer (GLenum target);
typedef void        Type_glGetIntegeri_v (GLenum target, GLuint index, GLint *data);
typedef void        Type_glDeleteBuffers (GLsizei n, const GLuint *buffers);
typedef void        Type_glClearNamedBufferData (GLuint buffer, GLenum internalformat, GLenum format, GLenum type, const void *data);
typedef void        Type_glDeleteFramebuffers (GLsizei n, const GLuint *framebuffers);
typedef void        Type_glDeleteRenderbuffers (GLsizei n, const GLuint *renderbuffers);
typedef void        Type_glBindTextureUnit (GLuint unit, GLuint texture);
typedef const GLubyte *Type_glGetStringi (GLenum name, GLuint index);
typedef void        Type_glGenerateTextureMipmap (GLuint texture);
typedef void        Type_glPatchParameteri(GLenum pname, GLint value);
typedef void        Type_glDeleteShader(GLuint shader);
typedef void        Type_glFramebufferTexture (GLenum target, GLenum attachment, GLuint texture, GLint level);
typedef void        Type_glTexImage3D (GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void *pixels);
typedef GLenum      Type_glCheckFramebufferStatus (GLenum target);
typedef void        Type_glUniform4f (GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
typedef void        Type_glUniform2f (GLint location, GLfloat v0, GLfloat v1);


// ---------------------------------------------------------------
// @Note: Function Pointers
#define OPENGL_FUNCTION(Name) Type_##Name *Name
OPENGL_FUNCTION(glCreateShader);
OPENGL_FUNCTION(glShaderSource);
OPENGL_FUNCTION(glCompileShader);
OPENGL_FUNCTION(glCreateProgram);
OPENGL_FUNCTION(glAttachShader);
OPENGL_FUNCTION(glLinkProgram);
OPENGL_FUNCTION(glGetProgramiv);
OPENGL_FUNCTION(glGetShaderInfoLog);
OPENGL_FUNCTION(glValidateProgram);
OPENGL_FUNCTION(glGetProgramInfoLog);
OPENGL_FUNCTION(glGenBuffers);
OPENGL_FUNCTION(glBindBuffer);
OPENGL_FUNCTION(glUniformMatrix4fv);
OPENGL_FUNCTION(glGetUniformLocation);
OPENGL_FUNCTION(glUseProgram);
OPENGL_FUNCTION(glUniform1i);
OPENGL_FUNCTION(glBufferData);
OPENGL_FUNCTION(glVertexAttribPointer);
OPENGL_FUNCTION(glGetAttribLocation);
OPENGL_FUNCTION(glEnableVertexAttribArray);
OPENGL_FUNCTION(glGenVertexArrays);
OPENGL_FUNCTION(glBindVertexArray);
OPENGL_FUNCTION(glBindAttribLocation);
OPENGL_FUNCTION(glDebugMessageCallbackARB);
OPENGL_FUNCTION(glDisableVertexAttribArray);
OPENGL_FUNCTION(glUniform3fv);
OPENGL_FUNCTION(glVertexAttribIPointer);
OPENGL_FUNCTION(glUniform4fv);
OPENGL_FUNCTION(glVertexAttribDivisor);
OPENGL_FUNCTION(glDrawElementsInstanced);
OPENGL_FUNCTION(glUniform1f);
OPENGL_FUNCTION(glUniform1fv);
OPENGL_FUNCTION(glGenFramebuffers);
OPENGL_FUNCTION(glBindFramebuffer);
OPENGL_FUNCTION(glFramebufferTexture2D);
OPENGL_FUNCTION(glTexStorage3D);
OPENGL_FUNCTION(glTexSubImage3D);
OPENGL_FUNCTION(glGenerateMipmap);
OPENGL_FUNCTION(glBindImageTexture);
OPENGL_FUNCTION(glClearTexImage);
OPENGL_FUNCTION(glDrawBuffers);
OPENGL_FUNCTION(glActiveTexture);
OPENGL_FUNCTION(glBindRenderbuffer);
OPENGL_FUNCTION(glRenderbufferStorage);
OPENGL_FUNCTION(glFramebufferRenderbuffer);
OPENGL_FUNCTION(glGenRenderbuffers);
OPENGL_FUNCTION(glBufferSubData);
OPENGL_FUNCTION(glBufferStorage);
OPENGL_FUNCTION(glBindBufferBase);
OPENGL_FUNCTION(glGetBufferSubData);
OPENGL_FUNCTION(glTexBuffer);
OPENGL_FUNCTION(glUniform1ui);
OPENGL_FUNCTION(glDispatchCompute);
OPENGL_FUNCTION(glMemoryBarrier);
OPENGL_FUNCTION(glMapBufferRange);
OPENGL_FUNCTION(glUnmapBuffer);
OPENGL_FUNCTION(glGetIntegeri_v);
OPENGL_FUNCTION(glDeleteBuffers);
OPENGL_FUNCTION(glClearNamedBufferData);
OPENGL_FUNCTION(glDeleteFramebuffers);
OPENGL_FUNCTION(glDeleteRenderbuffers);
OPENGL_FUNCTION(glBindTextureUnit);
OPENGL_FUNCTION(glGetStringi);
OPENGL_FUNCTION(glGenerateTextureMipmap);
OPENGL_FUNCTION(glPatchParameteri);
OPENGL_FUNCTION(glDeleteShader);
OPENGL_FUNCTION(glFramebufferTexture);
OPENGL_FUNCTION(glTexImage3D);
OPENGL_FUNCTION(glCheckFramebufferStatus);
OPENGL_FUNCTION(glUniform4f);
OPENGL_FUNCTION(glUniform2f);

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

    s32 time;

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

    s32 time;

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
};


// ---------------------------------------------------------------
// @Note: Function Declarations.
internal Opengl_Info opengl_get_info(Opengl *gl, b32 modern_context);
internal GLuint opengl_create_compute_program(Opengl *gl, const char *csrc);
internal GLuint opengl_create_program(Opengl *gl, const char *vsrc,const char *fsrc);
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
internal void opengl_init(Opengl *gl);

#endif // RTS_RENDERER_OPENGL_H
