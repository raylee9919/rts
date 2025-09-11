/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Sung Woo Lee $
   $Notice: (C) Copyright 2025 by Sung Woo Lee. All Rights Reserved. $
   ======================================================================== */


#include <stdio.h> // @TODO: Remove

global const char *g_shader_header = 
#include "shader/header.glsl"

global char g_shared[2048];


    
#define GET_UNIFORM_LOCATION(Program, Name) gl->Program.Name = glGetUniformLocation(gl->Program.id, #Name);
#define GL_FOR(ITER) for (u32 gl_iter = 0; gl_iter < ITER; ++gl_iter)

#define GL_DEBUG_CALLBACK(Name) void Name(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam)
typedef GL_DEBUG_CALLBACK(GLDEBUGPROC);
GL_DEBUG_CALLBACK(opengl_debug_callback)
{
    char *error = (char *)message;
    switch (severity) 
    {
        case GL_DEBUG_SEVERITY_LOW: 
        {

        } break;
        case GL_DEBUG_SEVERITY_MEDIUM: 
        {
            // Assert(0);
        } break;
        case GL_DEBUG_SEVERITY_HIGH: 
        {
            Assert(0);
        } break;
    }
}

internal Opengl_Info
opengl_get_info(Opengl *gl, b32 modern_context)
{
    Opengl_Info result = {};
    
    result.modern_context = modern_context;
    result.vendor         = (char *)glGetString(GL_VENDOR);
    result.renderer       = (char *)glGetString(GL_RENDERER);
    result.version        = (char *)glGetString(GL_VERSION);
    if (result.modern_context) {
        result.shading_language_version = (char *)glGetString(GL_SHADING_LANGUAGE_VERSION);
    } else {
        result.shading_language_version = "(none)";
    }
    
    if (glGetStringi)
    {
        GLint extension_count = 0;
        glGetIntegerv(GL_NUM_EXTENSIONS, &extension_count);
        for (GLint i = 0; i < extension_count; ++i)
        {
            char *ext_name = (char *)glGetStringi(GL_EXTENSIONS, i);
            
            if(0) {}
            else if(string_equal(ext_name, "GL_EXT_texture_sRGB")) { result.opengl_ext_texture_sgb=true; }
            else if(string_equal(ext_name, "GL_EXT_framebuffer_sRGB")) { result.opengl_ext_framebuffer_srgb=true; }
            else if(string_equal(ext_name, "GL_ARB_framebuffer_sRGB")) { result.opengl_ext_framebuffer_srgb=true; }
            else if(string_equal(ext_name, "GL_ARB_framebuffer_object")) { result.opengl_arb_framebuffer_object=true; }
            // @TODO: Is there some kind of ARB string to look for that indicates GL_EXT_texture_sRGB?
        }
    }
    
    char *major_at = result.version;
    char *minor_at = 0;
    for (char *at = result.version; *at; ++at) {
        if(at[0] == '.') {
            minor_at = at + 1;
            break;
        }
    }
    
    s32 major = 1;
    s32 minor = 0;
    if (minor_at) {
        major = s32_from_z(major_at);
        minor = s32_from_z(minor_at);
    }
    
    if ((major > 2) || ((major == 2) && (minor >= 1))) {
        // @NOTE: We _believe_ we have sRGB textures in 2.1 and above automatically.
        result.opengl_ext_texture_sgb = true;
    }
    
    if (major >= 3) {
        // @NOTE: We _believe_ we have framebuffer objects in 3.0 and above automatically.
        result.opengl_arb_framebuffer_object=true;
    }
    
    return result;
}

internal GLuint
opengl_create_compute_program(Opengl *gl, const char *csrc)
{
    GLuint program = 0;

    if (glCreateShader) 
    {
        GLuint cshader = glCreateShader(GL_COMPUTE_SHADER);
        const GLchar *cunit[] = { g_shader_header, g_shared, csrc };
        glShaderSource(cshader, array_count(cunit), (const GLchar **)cunit, 0);
        glCompileShader(cshader);

        program = glCreateProgram();
        glAttachShader(program, cshader);
        glLinkProgram(program);

        glValidateProgram(program);
        GLint linked = false;
        glGetProgramiv(program, GL_LINK_STATUS, &linked);
        if (!linked) 
        {
            GLsizei stub;

            GLchar clog[1024];
            glGetProgramInfoLog(cshader, sizeof(clog), &stub, clog);

            GLchar plog[1024];
            glGetProgramInfoLog(program, sizeof(plog), &stub, plog);

            Assert(!"compile/link error.");
        }

        glDeleteShader(cshader);
    } else {
        // TODO: handling.
    }
    
    return program;
}

internal GLuint
opengl_create_program(Opengl *gl, const char *vsrc,const char *fsrc)
{
    GLuint program = 0;

    if (glCreateShader) 
    {
        GLuint vshader = glCreateShader(GL_VERTEX_SHADER);
        const GLchar *vunit[] = { g_shader_header, g_shared, vsrc };
        glShaderSource(vshader, array_count(vunit), (const GLchar **)vunit, 0);
        glCompileShader(vshader);

        GLuint fshader = glCreateShader(GL_FRAGMENT_SHADER);
        const GLchar *funit[] = { g_shader_header, g_shared, fsrc };
        glShaderSource(fshader, array_count(funit), (const GLchar **)funit, 0);
        glCompileShader(fshader);

        program = glCreateProgram();
        glAttachShader(program, vshader);
        glAttachShader(program, fshader);
        glLinkProgram(program);

        glValidateProgram(program);
        GLint linked = false;
        glGetProgramiv(program, GL_LINK_STATUS, &linked);
        if (!linked) {
            GLsizei stub;

            GLchar vlog[1024];
            glGetShaderInfoLog(vshader, sizeof(vlog), &stub, vlog);

            GLchar flog[1024];
            glGetShaderInfoLog(fshader, sizeof(flog), &stub, flog);

            GLchar plog[1024];
            glGetProgramInfoLog(program, sizeof(plog), &stub, plog);

            Assert(!"compile/link error.");
        }

        glDeleteShader(vshader);
        glDeleteShader(fshader);
    } else {
        // @TODO: Error-Handling.
    }
    
    return program;
}

internal GLuint
opengl_create_program(Opengl *gl, const char *vsrc, const char *gsrc, const char *fsrc)
{
    GLuint program = 0;

    if (glCreateShader) 
    {
        GLuint vshader = glCreateShader(GL_VERTEX_SHADER);
        const GLchar *vunit[] = { g_shader_header, g_shared, vsrc };
        glShaderSource(vshader, array_count(vunit), (const GLchar **)vunit, 0);
        glCompileShader(vshader);

        GLuint gshader = glCreateShader(GL_GEOMETRY_SHADER);
        const GLchar *gunit[] = { g_shader_header, g_shared, gsrc };
        glShaderSource(gshader, array_count(gunit), (const GLchar **)gunit, 0);
        glCompileShader(gshader);

        GLuint fshader = glCreateShader(GL_FRAGMENT_SHADER);
        const GLchar *funit[] = { g_shader_header, g_shared, fsrc };
        glShaderSource(fshader, array_count(funit), (const GLchar **)funit, 0);
        glCompileShader(fshader);

        program = glCreateProgram();
        glAttachShader(program, vshader);
        glAttachShader(program, gshader);
        glAttachShader(program, fshader);
        glLinkProgram(program);

        glValidateProgram(program);
        GLint linked = false;
        glGetProgramiv(program, GL_LINK_STATUS, &linked);
        if (!linked) 
        {
            GLsizei stub;

            GLchar vlog[1024];
            glGetShaderInfoLog(vshader, sizeof(vlog), &stub, vlog);

            GLchar glog[1024];
            glGetShaderInfoLog(gshader, sizeof(glog), &stub, glog);

            GLchar flog[1024];
            glGetShaderInfoLog(fshader, sizeof(flog), &stub, flog);

            GLchar plog[1024];
            glGetProgramInfoLog(program, sizeof(plog), &stub, plog);

            Assert(!"compile/link error.");
        }

        glDeleteShader(vshader);
        glDeleteShader(gshader);
        glDeleteShader(fshader);
    } else {
        // @TODO: Error-Handling.
    }
    
    return program;
}

internal GLuint
opengl_create_tessellation_program(Opengl *gl, const char *vs, const char *tcs, const char *tes, const char *fs)
{
    GLuint program = 0;

    if (glCreateShader) 
    {
        GLuint vshader = glCreateShader(GL_VERTEX_SHADER);
        const GLchar *vunit[] = { g_shader_header, g_shared, vs };
        glShaderSource(vshader, array_count(vunit), (const GLchar **)vunit, 0);
        glCompileShader(vshader);

        GLuint tcshader = glCreateShader(GL_TESS_CONTROL_SHADER);
        const GLchar *tcsunit[] = { g_shader_header, g_shared, tcs };
        glShaderSource(tcshader, array_count(tcsunit), (const GLchar **)tcsunit, 0);
        glCompileShader(tcshader);

        GLuint teshader = glCreateShader(GL_TESS_EVALUATION_SHADER);
        const GLchar *tesunit[] = { g_shader_header, g_shared, tes };
        glShaderSource(teshader, array_count(tesunit), (const GLchar **)tesunit, 0);
        glCompileShader(teshader);

        GLuint fshader = glCreateShader(GL_FRAGMENT_SHADER);
        const GLchar *funit[] = { g_shader_header, g_shared, fs };
        glShaderSource(fshader, array_count(funit), (const GLchar **)funit, 0);
        glCompileShader(fshader);

        program = glCreateProgram();
        glAttachShader(program, vshader);
        glAttachShader(program, tcshader);
        glAttachShader(program, teshader);
        glAttachShader(program, fshader);
        glLinkProgram(program);

        glValidateProgram(program);
        GLint linked = false;
        glGetProgramiv(program, GL_LINK_STATUS, &linked);
        if (!linked) 
        {
            GLsizei stub;

            GLchar vlog[1024];
            glGetShaderInfoLog(vshader, sizeof(vlog), &stub, vlog);

            GLchar tcslog[1024];
            glGetShaderInfoLog(tcshader, sizeof(tcslog), &stub, tcslog);

            GLchar teslog[1024];
            glGetShaderInfoLog(teshader, sizeof(teslog), &stub, teslog);

            GLchar flog[1024];
            glGetShaderInfoLog(fshader, sizeof(flog), &stub, flog);

            GLchar plog[1024];
            glGetProgramInfoLog(program, sizeof(plog), &stub, plog);

            Assert(!"compile/link error.");
        }

        glDeleteShader(vshader);
        glDeleteShader(tcshader);
        glDeleteShader(teshader);
        glDeleteShader(fshader);
    } else {
        // @TODO: Error-Handling.
    }
    
    return program;
}

internal GLuint
opengl_create_tessellation_geometry_program(Opengl *gl, const char *vs, const char *tcs, const char *tes, const char *gs, const char *fs)
{
    GLuint program = 0;

    if (glCreateShader) 
    {
        GLuint vshader = glCreateShader(GL_VERTEX_SHADER);
        const GLchar *vunit[] = { g_shader_header, g_shared, vs };
        glShaderSource(vshader, array_count(vunit), (const GLchar **)vunit, 0);
        glCompileShader(vshader);

        GLuint tcshader = glCreateShader(GL_TESS_CONTROL_SHADER);
        const GLchar *tcsunit[] = { g_shader_header, g_shared, tcs };
        glShaderSource(tcshader, array_count(tcsunit), (const GLchar **)tcsunit, 0);
        glCompileShader(tcshader);

        GLuint teshader = glCreateShader(GL_TESS_EVALUATION_SHADER);
        const GLchar *tesunit[] = { g_shader_header, g_shared, tes };
        glShaderSource(teshader, array_count(tesunit), (const GLchar **)tesunit, 0);
        glCompileShader(teshader);

        GLuint fshader = glCreateShader(GL_FRAGMENT_SHADER);
        const GLchar *funit[] = { g_shader_header, g_shared, fs };
        glShaderSource(fshader, array_count(funit), (const GLchar **)funit, 0);
        glCompileShader(fshader);

        GLuint gshader = glCreateShader(GL_GEOMETRY_SHADER);
        const GLchar *gunit[] = { g_shader_header, g_shared, gs };
        glShaderSource(gshader, array_count(gunit), (const GLchar **)gunit, 0);
        glCompileShader(gshader);

        program = glCreateProgram();
        glAttachShader(program, vshader);
        glAttachShader(program, tcshader);
        glAttachShader(program, teshader);
        glAttachShader(program, fshader);
        glAttachShader(program, gshader);
        glLinkProgram(program);

        glValidateProgram(program);
        GLint linked = false;
        glGetProgramiv(program, GL_LINK_STATUS, &linked);
        if (!linked) 
        {
            GLsizei stub;

            GLchar vlog[1024];
            glGetShaderInfoLog(vshader, sizeof(vlog), &stub, vlog);

            GLchar tcslog[1024];
            glGetShaderInfoLog(tcshader, sizeof(tcslog), &stub, tcslog);

            GLchar teslog[1024];
            glGetShaderInfoLog(teshader, sizeof(teslog), &stub, teslog);

            GLchar gslog[1024];
            glGetShaderInfoLog(gshader, sizeof(gslog), &stub, gslog);

            GLchar flog[1024];
            glGetShaderInfoLog(fshader, sizeof(flog), &stub, flog);

            GLchar plog[1024];
            glGetProgramInfoLog(program, sizeof(plog), &stub, plog);

            Assert(!"compile/link error.");
        }

        glDeleteShader(vshader);
        glDeleteShader(tcshader);
        glDeleteShader(teshader);
        glDeleteShader(gshader);
        glDeleteShader(fshader);
    } else {
        // @TODO: Error-Handling.
    }
    
    return program;
}

internal void
opengl_alloc_texture(Opengl *gl, Bitmap *bitmap, GLenum wrapping, b32 generate_mipmap = false)
{
    glGenTextures(1, &bitmap->handle);
    glBindTexture(GL_TEXTURE_2D, bitmap->handle);

    if (bitmap->bits_per_channel == 8) {
        switch (bitmap->channel_count) 
        {
            case 1: 
            glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, bitmap->width, bitmap->height, 0, GL_RED, GL_UNSIGNED_BYTE, bitmap->memory);
            break;

            case 2: 
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RG8, bitmap->width, bitmap->height, 0, GL_RG, GL_UNSIGNED_BYTE, bitmap->memory);
            break;

            case 3: 
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, bitmap->width, bitmap->height, 0, GL_RGB, GL_UNSIGNED_BYTE, bitmap->memory);
            break;

            case 4: 
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, bitmap->width, bitmap->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, bitmap->memory);
            break;

            INVALID_DEFAULT_CASE;
        }
    } else if (bitmap->bits_per_channel == 16) {
        switch (bitmap->channel_count) {
            case 1: 
            glTexImage2D(GL_TEXTURE_2D, 0, GL_R16, bitmap->width, bitmap->height, 0, GL_RED, GL_UNSIGNED_SHORT, bitmap->memory);
            break;

            case 2: 
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16, bitmap->width, bitmap->height, 0, GL_RG, GL_UNSIGNED_SHORT, bitmap->memory);
            break;

            case 3: 
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16, bitmap->width, bitmap->height, 0, GL_RGB, GL_UNSIGNED_SHORT, bitmap->memory);
            break;

            case 4: 
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16, bitmap->width, bitmap->height, 0, GL_RGBA, GL_UNSIGNED_SHORT, bitmap->memory);
            break;

            INVALID_DEFAULT_CASE;
        }
    } else {
        INVALID_CODE_PATH;
    }

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapping);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapping);

    if (generate_mipmap) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glGenerateMipmap(GL_TEXTURE_2D);
        glGenerateTextureMipmap(bitmap->handle);
    } else {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    }
}

internal void
opengl_bind_texture(Opengl *gl, Bitmap *bitmap, b32 generate_mipmap = false)
{
    if (!bitmap) {
        bitmap = &gl->white_bitmap;
    }

    if (bitmap->handle) {
        glBindTexture(GL_TEXTURE_2D, bitmap->handle);
    } else {
        opengl_alloc_texture(gl, bitmap, GL_CLAMP_TO_EDGE, generate_mipmap);
        glBindTexture(GL_TEXTURE_2D, bitmap->handle);
    }
}

internal void
opengl_bind_atomic_counter(Opengl *gl, s32 id, s32 binding_point)
{
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, id);
    glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, binding_point, id);
}

internal void
opengl_gen_linear_buffer(Opengl *gl, u32 *buf, u32 *tex, GLenum format, size_t size)
{
    if (*buf) {
        glDeleteBuffers(1, buf);
    }
    glGenBuffers(1, buf);
    glBindBuffer(GL_TEXTURE_BUFFER, *buf);
    glBufferData(GL_TEXTURE_BUFFER, size, 0, GL_STATIC_DRAW);

    if (*tex) {
        glDeleteTextures(1, tex);
    }
    glGenTextures(1, tex);
    glBindTexture(GL_TEXTURE_BUFFER, *tex);
    glTexBuffer(GL_TEXTURE_BUFFER, format, *buf);

    glBindBuffer(GL_TEXTURE_BUFFER, 0);
}

internal void
opengl_set_flags_for_wireframe_mode(u32 *flags)
{
    *flags = (*flags) &= (~Pbr_Has_albedo);
    *flags = (*flags) &= (~Pbr_Has_normal);
    *flags = (*flags) &= (~Pbr_Has_roughness);
    *flags = (*flags) &= (~Pbr_Has_metalic);
    *flags = (*flags) &= (~Pbr_Has_emission);
    *flags = (*flags) &= (~Pbr_Has_orm);
    *flags = (*flags) |= (Pbr_No_Lighting);
}

internal void
gl_pbr_bind_texture_and_set_flags(Opengl *gl, Mesh *mesh, GLuint slot, GLenum wrapping, b32 mipmap, Pbr_Texture_Type type, u32 *flags)
{
    Bitmap *texture = &mesh->textures[type];
    if (texture->size) {
        switch(type)
        {
            case Pbr_Texture_Albedo:
            *flags = (*flags | Pbr_Has_albedo);
            break;

            case Pbr_Texture_Normal:
            *flags = (*flags | Pbr_Has_normal);
            break;

            case Pbr_Texture_Roughness:
            *flags = (*flags | Pbr_Has_roughness);
            break;

            case Pbr_Texture_Metalic:
            *flags = (*flags | Pbr_Has_metalic);
            break;

            case Pbr_Texture_Emission:
            *flags = (*flags | Pbr_Has_emission);
            break;

            case Pbr_Texture_Orm:
            *flags = (*flags | Pbr_Has_orm);
            break;

            INVALID_DEFAULT_CASE;
        }
        if (!texture->handle) {
            opengl_alloc_texture(gl, texture, wrapping, mipmap);
        }
        glBindTextureUnit(slot, texture->handle);
    }
}

internal void
opengl_compile_shaders(Opengl *gl)
{
    snprintf(g_shared, array_count(g_shared), R"(
    #define MAX_BONE_PER_VERTEX %u
    #define MAX_BONE_PER_MESH   %u
    #define MAX_LIGHTS          %u
    #define CSM_COUNT           %u

    #define Pbr_Has_albedo      %u
    #define Pbr_Has_normal      %u
    #define Pbr_Has_roughness   %u
    #define Pbr_Has_metalic     %u
    #define Pbr_Has_emission    %u
    #define Pbr_Has_orm         %u
    #define Pbr_No_Lighting     %u
    )", MAX_BONE_PER_VERTEX, MAX_BONE_PER_MESH, MAX_LIGHTS, CSM_COUNT,
             Pbr_Has_albedo,
             Pbr_Has_normal,
             Pbr_Has_roughness,
             Pbr_Has_metalic,
             Pbr_Has_emission,
             Pbr_Has_orm,
             Pbr_No_Lighting
            );

    const char *sprite_vshader = 
        #include "shader/sprite_vs.glsl"
    const char *sprite_fshader = 
        #include "shader/sprite_fs.glsl"

    const char *pbr_vs = 
        #include "shader/pbr_vs.glsl"
    const char *pbr_fs = 
        #include "shader/pbr_fs.glsl"

    const char *skybox_vs = 
        #include "shader/skybox_vs.glsl"
    const char *skybox_fs = 
        #include "shader/skybox_fs.glsl"

    const char *ground_vs = 
        #include "shader/ground_vs.glsl"
    const char *ground_tcs = 
        #include "shader/ground_tcs.glsl"
    const char *ground_tes = 
        #include "shader/ground_tes.glsl"

    const char *csm_vs = 
        #include "shader/csm_vs.glsl"
    const char *csm_gs = 
        #include "shader/csm_gs.glsl"
    const char *csm_fs = 
        #include "shader/csm_fs.glsl"

    const char *simple_vs = 
        #include "shader/simple_vs.glsl"
    const char *simple_fs = 
        #include "shader/simple_fs.glsl"

    const char *blt_vs = 
        #include "shader/blt_vs.glsl"
    const char *blt_fs = 
        #include "shader/blt_fs.glsl"

    const char *circle_vs = 
        #include "shader/circle_vs.glsl"
    const char *circle_fs = 
        #include "shader/circle_fs.glsl"



    glDeleteShader(gl->sprite_program.id);
    gl->sprite_program.id = opengl_create_program(gl, sprite_vshader, sprite_fshader);
    GET_UNIFORM_LOCATION(sprite_program, mvp);
    GET_UNIFORM_LOCATION(sprite_program, color);
    GET_UNIFORM_LOCATION(sprite_program, texture);

    glDeleteShader(gl->pbr_program.id);
    gl->pbr_program.id = opengl_create_program(gl, pbr_vs, pbr_fs);
    GET_UNIFORM_LOCATION(pbr_program, world_transform);
    GET_UNIFORM_LOCATION(pbr_program, VP);
    GET_UNIFORM_LOCATION(pbr_program, is_skeletal);
    GET_UNIFORM_LOCATION(pbr_program, uv_scale);
    GET_UNIFORM_LOCATION(pbr_program, entity_id);
    GET_UNIFORM_LOCATION(pbr_program, hot_entity_id);
    GET_UNIFORM_LOCATION(pbr_program, active_entity_id);
    GET_UNIFORM_LOCATION(pbr_program, bone_transforms);
    GET_UNIFORM_LOCATION(pbr_program, eye_position);
    GET_UNIFORM_LOCATION(pbr_program, flags);
    GET_UNIFORM_LOCATION(pbr_program, wireframe_color);
    GET_UNIFORM_LOCATION(pbr_program, tint);
    GET_UNIFORM_LOCATION(pbr_program, shadowmap_view_projs);
    GET_UNIFORM_LOCATION(pbr_program, to_light);
    GET_UNIFORM_LOCATION(pbr_program, csm_view);
    GET_UNIFORM_LOCATION(pbr_program, csm_z_spans);
    GET_UNIFORM_LOCATION(pbr_program, time);

    glDeleteShader(gl->ground_program.id);
    gl->ground_program.id = opengl_create_tessellation_program(gl, ground_vs, ground_tcs, ground_tes, pbr_fs);
    GET_UNIFORM_LOCATION(ground_program, model);
    GET_UNIFORM_LOCATION(ground_program, eye_position);
    GET_UNIFORM_LOCATION(ground_program, view_proj);
    GET_UNIFORM_LOCATION(ground_program, elevation);
    GET_UNIFORM_LOCATION(ground_program, flags);
    GET_UNIFORM_LOCATION(ground_program, wireframe_color);
    GET_UNIFORM_LOCATION(ground_program, shadowmap_view_projs);
    GET_UNIFORM_LOCATION(ground_program, to_light);
    GET_UNIFORM_LOCATION(ground_program, csm_view);
    GET_UNIFORM_LOCATION(ground_program, csm_z_spans);
    GET_UNIFORM_LOCATION(ground_program, entity_id);
    GET_UNIFORM_LOCATION(ground_program, hot_entity_id);
    GET_UNIFORM_LOCATION(ground_program, active_entity_id);
    GET_UNIFORM_LOCATION(ground_program, time);

    glDeleteShader(gl->skybox_program.id);
    gl->skybox_program.id = opengl_create_program(gl, skybox_vs, skybox_fs);
    GET_UNIFORM_LOCATION(skybox_program, view_proj);

    glDeleteShader(gl->shadowmap_program.id);
    gl->shadowmap_program.id = opengl_create_program(gl, csm_vs, csm_gs, csm_fs);
    GET_UNIFORM_LOCATION(shadowmap_program, world_transform);
    GET_UNIFORM_LOCATION(shadowmap_program, VP);
    GET_UNIFORM_LOCATION(shadowmap_program, is_skeletal);
    GET_UNIFORM_LOCATION(shadowmap_program, bone_transforms);
    GET_UNIFORM_LOCATION(shadowmap_program, light_view_projs);

#if 0
    glDeleteShader(gl->shadowmap_ground_program.id);
    gl->shadowmap_ground_program.id = opengl_create_tessellation_geometry_program(gl, ground_vs, ground_tcs, ground_tes, csm_gs, pbr_fs);
    GET_UNIFORM_LOCATION(shadowmap_ground_program, model);
    GET_UNIFORM_LOCATION(shadowmap_ground_program, eye_position);
    GET_UNIFORM_LOCATION(shadowmap_ground_program, view_proj);
    GET_UNIFORM_LOCATION(shadowmap_ground_program, elevation);
    GET_UNIFORM_LOCATION(shadowmap_ground_program, light_view_projs);
#endif

    glDeleteShader(gl->simple_program.id);
    gl->simple_program.id = opengl_create_program(gl, simple_vs, simple_fs);
    GET_UNIFORM_LOCATION(simple_program, VP);
    GET_UNIFORM_LOCATION(simple_program, color);

    glDeleteShader(gl->blt_program.id);
    gl->blt_program.id = opengl_create_program(gl, blt_vs, blt_fs);

    glDeleteShader(gl->circle_program.id);
    gl->circle_program.id = opengl_create_program(gl, circle_vs, circle_fs);
    GET_UNIFORM_LOCATION(circle_program, model);
    GET_UNIFORM_LOCATION(circle_program, view_proj);
    GET_UNIFORM_LOCATION(circle_program, radius);
}

internal Render_Commands *
opengl_begin_frame(Opengl *gl, v2u window_dim, v2u render_dim)
{
    Render_Commands *frame = &gl->render_commands;

    frame->window_dim = window_dim;
    frame->render_dim  = render_dim;

    frame->push_buffer_size = gl->push_buffer_size;
    frame->push_buffer_base = gl->push_buffer;
    frame->push_buffer_used = 0;

    return frame;
}

internal void
opengl_end_frame(Opengl *gl, Render_Commands *frame)
{
    u32 window_width  = frame->window_dim.w;
    u32 window_height = frame->window_dim.h;
    u32 render_width  = frame->render_dim.w;
    u32 render_height = frame->render_dim.h;


    { // @Frame texture
        glBindFramebuffer(GL_FRAMEBUFFER, gl->fbo);

        glDeleteRenderbuffers(1, &gl->depth_buffer);
        glGenRenderbuffers(1, &gl->depth_buffer);
        glBindRenderbuffer(GL_RENDERBUFFER, gl->depth_buffer);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, render_width, render_height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, gl->depth_buffer);

        glDeleteTextures(1, &gl->color_texture);
        glGenTextures(1, &gl->color_texture);
        glBindTexture(GL_TEXTURE_2D, gl->color_texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, render_width, render_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gl->color_texture, 0);

        glDeleteTextures(1, &gl->id_texture);
        glGenTextures(1, &gl->id_texture);
        glBindTexture(GL_TEXTURE_2D, gl->id_texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R32UI, render_width, render_height, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        v4 noentity = V4(0);
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, (GLfloat *)&noentity);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gl->id_texture, 0);

        const GLenum attachments[] = {
            GL_COLOR_ATTACHMENT0,
            GL_COLOR_ATTACHMENT1,
        };
        glDrawBuffers(array_count(attachments), attachments);

        Assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindBuffer(GL_ARRAY_BUFFER, gl->vbo);

    glViewport(0, 0, window_width, window_height);

    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // @TODO: Robust Scissor Test
    //glEnable(GL_SCISSOR_TEST);
    //glScissor(0, 0, window_width, window_height);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glClearDepth(1.0f);
    glClear(GL_DEPTH_BUFFER_BIT);

    glClearColor(0,0,0,0);
    glClear(GL_COLOR_BUFFER_BIT);

    glDisable(GL_MULTISAMPLE);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);


    // @Csm Frustum
    v3 *csm_frustum_positions = frame->csm_frustum_positions;
    f32 distance_between_near_and_far = distance(0.5f * (csm_frustum_positions[0] + csm_frustum_positions[3]),
                                                 0.5f * (csm_frustum_positions[4] + csm_frustum_positions[7]));
    const u32 frustum_point_count = 8 + (CSM_COUNT - 1) * 4;
    v3 frustum_positions[frustum_point_count] = {};
    frustum_positions[0] = csm_frustum_positions[0];
    frustum_positions[3] = csm_frustum_positions[3];
    frustum_positions[1] = csm_frustum_positions[1];
    frustum_positions[2] = csm_frustum_positions[2];
    frustum_positions[frustum_point_count - 4] = csm_frustum_positions[4];
    frustum_positions[frustum_point_count - 1] = csm_frustum_positions[7];
    frustum_positions[frustum_point_count - 3] = csm_frustum_positions[5];
    frustum_positions[frustum_point_count - 2] = csm_frustum_positions[6];

    // @NOTE: If you change CSM_COUNT, you need to add interpolation value in here!
    const f32 frustum_z_weights[CSM_COUNT - 1] = {
        0.25f, 0.50f, 0.75f
    };
    f32 csm_z_spans[CSM_COUNT];
    for (u32 i = 0; i < CSM_COUNT; ++i) {
        csm_z_spans[i] = distance_between_near_and_far * frustum_z_weights[i];
    }

    for (u32 div = 0; div < CSM_COUNT - 1; ++div) {
        for (u32 i = 0; i < 4; ++i) {
            frustum_positions[4 + div*4 + i] = lerp(frustum_positions[i], frustum_z_weights[div], frustum_positions[frustum_point_count - (4 - i)]);
        }
    }

    u32 csm_frustum_offset[] = {
        0,1,2,2,1,3,
        4,0,6,6,0,2,
        6,2,7,7,2,3,
        1,5,3,3,5,7,
        0,4,1,1,4,5
    };
    u32 csm_outline_offset[] = {
        0,1,1,3,3,2,2,0,
        4,0,0,2,2,6,6,4,
        6,2,2,3,3,7,7,6,
        1,3,3,7,7,5,5,1,
        4,0,0,1,1,5,5,4,
    };
    const u32 csm_frustum_index_count = array_count(csm_frustum_offset);
    u32 csm_frustum_indices[csm_frustum_index_count * (CSM_COUNT + 1)];
    for (u32 level = 0; level < CSM_COUNT + 1; ++level) {
        for (u32 i = 0; i < csm_frustum_index_count; ++i) {
            csm_frustum_indices[level * csm_frustum_index_count + i] = 4 * level + csm_frustum_offset[i];
        }
    }
    const u32 csm_outline_index_count = array_count(csm_outline_offset);
    u32 csm_outline_indices[csm_outline_index_count * CSM_COUNT];
    for (u32 level = 0; level < CSM_COUNT; ++level) {
        for (u32 i = 0; i < csm_outline_index_count; ++i) {
            csm_outline_indices[level * csm_outline_index_count + i] = 4 * level + csm_outline_offset[i];
        }
    }

    m4x4 light_view_projs[CSM_COUNT];
    for (u32 level = 0; level < CSM_COUNT; ++level) {
        if (frame->csm_varient_method) {
            v3 center = {};
            for (u32 i = 0; i < 8; ++i) {
                center += frustum_positions[level*4 + i];
            }
            center *= 0.125f;
            m4x4 light_view = lookat(center + frame->csm_to_light, center, v3{0,1,0}); // @TODO: Fit z?

            v3 min = V3(F32_MAX);
            v3 max = V3(-F32_MAX);
            for (u32 i = 0; i < 8; ++i) {
                v3 lp = (light_view * V4(frustum_positions[level*4 + i], 1)).xyz;
                min.x = min(min.x, lp.x);
                min.y = min(min.y, lp.y);
                min.z = min(min.z, lp.z);
                max.x = max(max.x, lp.x);
                max.y = max(max.y, lp.y);
                max.z = max(max.z, lp.z);
            }

            f32 depth = max.z - min.z; // @TODO: Fit z?

            m4x4 light_proj = ortho(min.x, max.x, min.y, max.y, -depth*2.0, depth*2.0);
            light_view_projs[level] = light_proj * light_view;
        } else {
            v3 A = frustum_positions[level*4 + 0];
            v3 B = frustum_positions[level*4 + 3];
            v3 C = frustum_positions[level*4 + 4];
            v3 D = frustum_positions[level*4 + 7];

            f32 a = distance(A, B);
            f32 b = distance(C, D);
            v3 AB = (A+B)*0.5f;
            v3 CD = (C+D)*0.5f;
            f32 h = distance(AB, CD);
            f32 k = (4*h*h + b*b - a*a) / (8*h);
            f32 r = sqrt(k*k + a*a*0.25f);
            f32 t = map(k, 0, h);
            v3 c = lerp(AB, t, CD);

            m4x4 light_view = lookat(c + frame->csm_to_light * r, c, V3(0,1,0));
            m4x4 light_proj = ortho(-r, r, -r, r, -2*r, 2*2*r); // @TODO: Constant min and max depths
            light_view_projs[level] = light_proj * light_view;
        }
    }


    { // @Shadowmap
        glViewport(0, 0, SHADOWMAP_RESOLUTION, SHADOWMAP_RESOLUTION);
        glBindFramebuffer(GL_FRAMEBUFFER, gl->shadowmap_fbo);
        glClear(GL_DEPTH_BUFFER_BIT);

        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glFrontFace(GL_CCW);

        m4x4 identity_view_proj = identity();

        Shadowmap_Program *shadowmap_program = &gl->shadowmap_program;
        glUseProgram(shadowmap_program->id);

        glUniformMatrix4fv(shadowmap_program->light_view_projs, CSM_COUNT, true, (GLfloat *)light_view_projs);

        for (u8 *buffer_at = frame->push_buffer_base;
             buffer_at < frame->push_buffer_base + frame->push_buffer_used;)
        {
            Render_Group *group = (Render_Group *)buffer_at;
            buffer_at += sizeof(Render_Group);

            for (u8 *group_at = buffer_at;
                 group_at < group->base + group->used;)
            {
                Render_Entity_Header *entity = (Render_Entity_Header *)group_at;
                group_at += entity->size;
                switch (entity->type)
                {
                    case eRender_Mesh: 
                    {
                        Render_Mesh *piece = (Render_Mesh *)entity;
                        Mesh *mesh = piece->mesh;

                        glUniformMatrix4fv(shadowmap_program->world_transform, 1, true, &piece->world_transform.e[0][0]);
                        glUniformMatrix4fv(shadowmap_program->VP, 1, GL_TRUE, &identity_view_proj.e[0][0]);
                        glUniform1i(shadowmap_program->is_skeletal, piece->animation_transforms ? 1 : 0);
                        if (piece->animation_transforms) {
                            glUniformMatrix4fv(shadowmap_program->bone_transforms, MAX_BONE_PER_MESH, true, (GLfloat *)piece->animation_transforms);
                        }

                        glEnableVertexAttribArray(0);
                        glEnableVertexAttribArray(5);
                        glEnableVertexAttribArray(6);

                        glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(Vertex), (GLvoid *)(offsetof(Vertex, position)));
                        glVertexAttribIPointer(5, MAX_BONE_PER_VERTEX, GL_INT, sizeof(Vertex), (GLvoid *)(offsetof(Vertex, node_ids)));
                        glVertexAttribPointer(6, MAX_BONE_PER_VERTEX, GL_FLOAT, false, sizeof(Vertex), (GLvoid *)(offsetof(Vertex, node_weights)));

                        glBufferData(GL_ARRAY_BUFFER, mesh->vertex_count * sizeof(Vertex), mesh->vertices, GL_DYNAMIC_DRAW);
                        glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh->index_count * sizeof(u32), mesh->indices, GL_DYNAMIC_DRAW);

                        glDrawElements(GL_TRIANGLES, mesh->index_count, GL_UNSIGNED_INT, (void *)0);

                        glDisableVertexAttribArray(0);
                        glDisableVertexAttribArray(5);
                        glDisableVertexAttribArray(6);
                    } break;

                    default: 
                    {
                    } break;
                }
            }
            buffer_at += group->capacity;
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, window_width, window_height);
    }

    { // @Skybox
        glDisable(GL_CULL_FACE);
        if (!gl->skybox_texture) {
            glGenTextures(1, &gl->skybox_texture);
            glBindTexture(GL_TEXTURE_CUBE_MAP, gl->skybox_texture);
            for (u32 i = 0; i < 6; ++i) {
                Bitmap *texture = frame->skybox_textures[i];
                // @Temporary: 
                glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, texture->width, texture->height, 0, GL_RGB, GL_UNSIGNED_BYTE, texture->memory);

                glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE); 
            }
        } else {
            glBindTexture(GL_TEXTURE_CUBE_MAP, gl->skybox_texture);
        }

        Skybox_Program *skybox_program = &gl->skybox_program;
        glUseProgram(skybox_program->id);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(Vertex), (GLvoid *)(offsetof(Vertex, position)));

        glUniformMatrix4fv(skybox_program->view_proj, 1, GL_TRUE, &frame->skybox_eye_view_proj.e[0][0]);

        Mesh *mesh = frame->skybox_mesh;
        glBufferData(GL_ARRAY_BUFFER, mesh->vertex_count * sizeof(Vertex), mesh->vertices, GL_DYNAMIC_DRAW);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh->index_count * sizeof(u32), mesh->indices, GL_DYNAMIC_DRAW);

        glDrawElements(GL_TRIANGLES, mesh->index_count, GL_UNSIGNED_INT, (void *)0);

        glDisableVertexAttribArray(0);
        glEnable(GL_CULL_FACE);
    }

    { // @Framebuffer Texture
        glBindFramebuffer(GL_FRAMEBUFFER, gl->fbo);
        glViewport(0, 0, render_width, render_height);

        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);
        glClearDepth(1.0f);
        glClear(GL_DEPTH_BUFFER_BIT);

        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glFrontFace(GL_CCW);

        { // @Pbr
            Pbr_Program *pbr_program = &gl->pbr_program;
            glUseProgram(pbr_program->id);

            for (u8 *buffer_at = frame->push_buffer_base;
                 buffer_at < frame->push_buffer_base + frame->push_buffer_used;)
            {
                Render_Group *group = (Render_Group *)buffer_at;
                buffer_at += sizeof(Render_Group);

                for (u8 *group_at = buffer_at;
                     group_at < group->base + group->used;)
                {
                    Render_Entity_Header *entity = (Render_Entity_Header *)group_at;
                    group_at += entity->size;
                    switch (entity->type)
                    {
                        case eRender_Mesh: 
                        {
                            Render_Mesh *piece = (Render_Mesh *)entity;
                            Mesh *mesh = piece->mesh;

                            glUniformMatrix4fv(pbr_program->VP, 1, GL_TRUE, &frame->main_view_proj.e[0][0]);
                            glUniform1i(pbr_program->is_skeletal, piece->animation_transforms ? 1 : 0);
                            glUniformMatrix4fv(pbr_program->world_transform, 1, true, &piece->world_transform.e[0][0]);
                            glUniform2f(pbr_program->uv_scale, piece->uv_scale.x, piece->uv_scale.y);
                            glUniform3fv(pbr_program->eye_position, 1, (GLfloat *)&frame->main_eye_position);
                            glUniformMatrix4fv(pbr_program->csm_view, 1, true, &frame->csm_view.e[0][0]);
                            glUniform1ui(pbr_program->entity_id, piece->entity_id);
                            glUniform1ui(pbr_program->hot_entity_id, frame->hot_entity_id);
                            glUniform1ui(pbr_program->active_entity_id, frame->active_entity_id);
                            glUniform4fv(pbr_program->wireframe_color, 1, (GLfloat *)&frame->wireframe_color);
                            glUniform4fv(pbr_program->tint, 1, (GLfloat *)&piece->tint);
                            glUniformMatrix4fv(pbr_program->shadowmap_view_projs, CSM_COUNT, true, &light_view_projs[0].e[0][0]);
                            glUniform3fv(pbr_program->to_light, 1, (GLfloat *)&frame->csm_to_light);
                            glUniform1fv(pbr_program->csm_z_spans, CSM_COUNT, csm_z_spans);
                            glUniform1f(pbr_program->time, frame->time);
                            glBindTextureUnit(6, gl->shadowmaps);

                            u32 flags = 0;
                            b32 generatemipmap = true;
                            gl_pbr_bind_texture_and_set_flags(gl, mesh, 0, GL_REPEAT, generatemipmap, Pbr_Texture_Albedo, &flags);
                            gl_pbr_bind_texture_and_set_flags(gl, mesh, 1, GL_REPEAT, generatemipmap, Pbr_Texture_Normal, &flags);
                            gl_pbr_bind_texture_and_set_flags(gl, mesh, 2, GL_REPEAT, generatemipmap, Pbr_Texture_Roughness, &flags);
                            gl_pbr_bind_texture_and_set_flags(gl, mesh, 3, GL_REPEAT, generatemipmap, Pbr_Texture_Metalic, &flags);
                            gl_pbr_bind_texture_and_set_flags(gl, mesh, 4, GL_REPEAT, generatemipmap, Pbr_Texture_Emission, &flags);
                            gl_pbr_bind_texture_and_set_flags(gl, mesh, 5, GL_REPEAT, generatemipmap, Pbr_Texture_Orm, &flags);

                            if (frame->wireframe_mode) {
                                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                                opengl_set_flags_for_wireframe_mode(&flags);
                            }
                            glUniform1ui(pbr_program->flags, flags);

                            GL_FOR(7) { glEnableVertexAttribArray(gl_iter); }

                            glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(Vertex), (GLvoid *)(offsetof(Vertex, position)));
                            glVertexAttribPointer(1, 3, GL_FLOAT, false, sizeof(Vertex), (GLvoid *)(offsetof(Vertex, normal)));
                            glVertexAttribPointer(2, 2, GL_FLOAT, false, sizeof(Vertex), (GLvoid *)(offsetof(Vertex, uv)));
                            glVertexAttribPointer(3, 4, GL_FLOAT, true,  sizeof(Vertex), (GLvoid *)(offsetof(Vertex, color)));
                            glVertexAttribPointer(4, 3, GL_FLOAT, false, sizeof(Vertex), (GLvoid *)(offsetof(Vertex, tangent)));
                            glVertexAttribIPointer(5, MAX_BONE_PER_VERTEX, GL_INT, sizeof(Vertex), (GLvoid *)(offsetof(Vertex, node_ids)));
                            glVertexAttribPointer(6, MAX_BONE_PER_VERTEX, GL_FLOAT, false, sizeof(Vertex), (GLvoid *)(offsetof(Vertex, node_weights)));

                            glBufferData(GL_ARRAY_BUFFER, mesh->vertex_count * sizeof(Vertex), mesh->vertices, GL_DYNAMIC_DRAW);
                            glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh->index_count * sizeof(u32), mesh->indices, GL_DYNAMIC_DRAW);

                            if (piece->animation_transforms) {
                                glUniformMatrix4fv(pbr_program->bone_transforms, MAX_BONE_PER_MESH, true, (GLfloat *)piece->animation_transforms);
                            }

                            glDrawElements(GL_TRIANGLES, mesh->index_count, GL_UNSIGNED_INT, (void *)0);

                            // Seems like a perf loss. -Ray
                            if (piece->entity_id != 0 &&
                                frame->active_entity_id == piece->entity_id) 
                            {
                                glDisable(GL_DEPTH_TEST);
                                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                                opengl_set_flags_for_wireframe_mode(&flags);
                                glUniform1ui(pbr_program->flags, flags);
                                glUniform4f(pbr_program->wireframe_color, 1.0f, 1.0f, 0.0f, 1.0f);
                                glDrawElements(GL_TRIANGLES, mesh->index_count, GL_UNSIGNED_INT, (void *)0);
                                glEnable(GL_DEPTH_TEST);
                            }

                            GL_FOR(7) { glDisableVertexAttribArray(gl_iter); }
                            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                        } break;

                        default: 
                        {
                        } break;
                    }
                }
                buffer_at += group->capacity;
            }
        }

        { // @Triangles
            Simple_Program *simple_program = &gl->simple_program;
            glUseProgram(simple_program->id);

            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

            for (u8 *buffer_at = frame->push_buffer_base;
                 buffer_at < frame->push_buffer_base + frame->push_buffer_used;)
            {
                Render_Group *group = (Render_Group *)buffer_at;
                buffer_at += sizeof(Render_Group);

                for (u8 *group_at = buffer_at;
                     group_at < group->base + group->used;)
                {
                    Render_Entity_Header *entity = (Render_Entity_Header *)group_at;
                    group_at += entity->size;
                    switch (entity->type) {
                        case eRender_Triangles: {
                            Render_Triangles *piece = (Render_Triangles *)entity;

                            glEnableVertexAttribArray(0);

                            glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(Vertex), (GLvoid *)(offsetof(Vertex, position)));
                            glBufferData(GL_ARRAY_BUFFER, piece->vertexcount * sizeof(Vertex), piece->vertices, GL_DYNAMIC_DRAW);

                            glUniformMatrix4fv(simple_program->VP, 1, GL_TRUE, &frame->main_view_proj.e[0][0]);
                            glUniform4fv(simple_program->color, 1, (GLfloat *)&piece->color);

                            glBufferData(GL_ELEMENT_ARRAY_BUFFER, piece->numtri*3*sizeof(u32), piece->indices, GL_DYNAMIC_DRAW);

                            glDrawElements(GL_TRIANGLES, piece->numtri*3, GL_UNSIGNED_INT, 0);

                            glDisableVertexAttribArray(0);
                        } break;

                        default: { 
                        } break;
                    }
                }
                buffer_at += group->capacity;
            }


            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }

        { // @Unit Circle
            Circle_Program *circle_program = &gl->circle_program;
            glUseProgram(circle_program->id);

            glEnableVertexAttribArray(0);

            v3 p[4] = {
                v3{-1, 0.1f,-1},
                v3{-1, 0.1f, 1},
                v3{ 1, 0.1f,-1},
                v3{ 1, 0.1f, 1}
            };

            u32 indices[6] = {0,1,2,2,1,3};

            glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(v3), 0);
            glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(v3), p, GL_DYNAMIC_DRAW);

            glUniformMatrix4fv(circle_program->model, 1, GL_TRUE, &frame->debug_transform.e[0][0]);
            glUniformMatrix4fv(circle_program->view_proj, 1, GL_TRUE, &frame->main_view_proj.e[0][0]);
            glUniform1f(circle_program->radius, frame->debug_radius);

            glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6*sizeof(u32), indices, GL_DYNAMIC_DRAW);

            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

            glDisableVertexAttribArray(0);
        }

        { // @Line
            Simple_Program *simple_program = &gl->simple_program;
            glUseProgram(simple_program->id);

            glDisable(GL_DEPTH_TEST);

            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

            for (u8 *buffer_at = frame->push_buffer_base;
                 buffer_at < frame->push_buffer_base + frame->push_buffer_used;)
            {
                Render_Group *group = (Render_Group *)buffer_at;
                buffer_at += sizeof(Render_Group);

                for (u8 *group_at = buffer_at;
                     group_at < group->base + group->used;)
                {
                    Render_Entity_Header *entity = (Render_Entity_Header *)group_at;
                    group_at += entity->size;
                    switch (entity->type) {
                        case eRender_Line: {
                            Render_Line *piece = (Render_Line *)entity;

                            glEnableVertexAttribArray(0);

                            glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(v3), 0);
                            glBufferData(GL_ARRAY_BUFFER, 2 * sizeof(v3), piece->p, GL_DYNAMIC_DRAW);

                            glUniformMatrix4fv(simple_program->VP, 1, GL_TRUE, &frame->main_view_proj.e[0][0]);
                            glUniform4fv(simple_program->color, 1, (GLfloat *)&piece->color);

                            u32 indices[2] = {0,1};
                            glBufferData(GL_ELEMENT_ARRAY_BUFFER, 2*sizeof(u32), indices, GL_DYNAMIC_DRAW);

                            glDrawElements(GL_LINES, 2, GL_UNSIGNED_INT, 0);

                            glDisableVertexAttribArray(0);
                        } break;

                        default: { 
                        } break;
                    }
                }
                buffer_at += group->capacity;
            }

            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            glEnable(GL_DEPTH_TEST);
        }

        { // @Mouse Picking
            if (!frame->input.interacted_ui) { // @Temporary
                glBindTexture(GL_TEXTURE_2D, gl->id_texture);

                u32 entity_id;
                s32 mousex = s32(frame->input.mouse.position.x);
                s32 mousey = s32(frame->input.mouse.position.y);
                glReadBuffer(GL_COLOR_ATTACHMENT1);
                glReadPixels(mousex, mousey, 1, 1, GL_RED_INTEGER, GL_UNSIGNED_INT, &entity_id);
                
                if (frame->input.mouse.is_down[Mouse_Left] && frame->input.mouse.toggle[Mouse_Left]) {
                    frame->toggled_down_mouse_position = frame->input.mouse.position;
                    frame->toggled_down_entity_id = entity_id;
                } else if (!frame->input.mouse.is_down[Mouse_Left] && frame->input.mouse.toggle[Mouse_Left]) {
                    if (frame->toggled_down_entity_id == entity_id &&
                        distance(frame->toggled_down_mouse_position, frame->input.mouse.position) < 1.0f) {
                        frame->active_entity_id = entity_id;
                    }
                }

                glBindTexture(GL_TEXTURE_2D, 0);
            }
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, window_width, window_height);
    }


    { // @Blt
        glDisable(GL_DEPTH_TEST);
        Blt_Program *program = &gl->blt_program;
        s32 pid = program->id;
        glUseProgram(pid);

        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(2);

        const Textured_Vertex vertices[] = {
            {v3{ 1,-1, 0}, v2{1,0}},
            {v3{ 1, 1, 0}, v2{1,1}},
            {v3{-1,-1, 0}, v2{0,0}},
            {v3{-1, 1, 0}, v2{0,1}}
        };

        glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(Textured_Vertex), (GLvoid *)offsetof(Textured_Vertex, pos));
        glVertexAttribPointer(2, 2, GL_FLOAT, false, sizeof(Textured_Vertex), (GLvoid *)offsetof(Textured_Vertex, uv));

        glBindTextureUnit(0, gl->color_texture);

        glBufferData(GL_ARRAY_BUFFER, array_count(vertices) * sizeof(*vertices), vertices, GL_STATIC_DRAW);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(2);
    }

    // @Csm Frustum
    if (frame->draw_csm_frustum) 
    {
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        glUseProgram(gl->simple_program.id);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(v3), (GLvoid *)0);

        v4 colors[] = {
            v4{1,0,1,0.1f},
            v4{0,1,0,0.1f},
            v4{0,0,1,0.1f},
        };
        if (frame->draw_csm_sphere) {
            for (u32 i = 0; i < array_count(colors); ++i) {
                colors[i].a = 0.01f;
            }
        }

        glUniformMatrix4fv(gl->simple_program.VP, 1, true, &frame->main_view_proj.e[0][0]);
        glBufferData(GL_ARRAY_BUFFER, array_count(frustum_positions) * sizeof(*frustum_positions), frustum_positions, GL_DYNAMIC_DRAW);
        for (u32 i = 0; i < CSM_COUNT; ++i) {
            glUniform4fv(gl->simple_program.color, 1, (GLfloat *)&colors[i % array_count(colors)]);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, csm_frustum_index_count * sizeof(u32), csm_frustum_indices + csm_frustum_index_count*i, GL_DYNAMIC_DRAW);
            glDrawElements(GL_TRIANGLES, csm_frustum_index_count, GL_UNSIGNED_INT, (void *)0);
        }

        v4 solid_black = V4(V3(0),1);
        glUniform4fv(gl->simple_program.color, 1, (GLfloat *)&solid_black);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, array_count(csm_outline_indices) * sizeof(u32), csm_outline_indices, GL_DYNAMIC_DRAW);
        glDrawElements(GL_LINES, array_count(csm_outline_indices), GL_UNSIGNED_INT, (void *)0);

        glDisableVertexAttribArray(0);
        glEnable(GL_CULL_FACE);
        glEnable(GL_DEPTH_TEST);
    }

    { // @Orthographic
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glClearDepth(1);
        glClear(GL_DEPTH_BUFFER_BIT);

        Sprite_Program *program = &gl->sprite_program;
        s32 pid = program->id;
        glUseProgram(pid);

        for (u8 *buffer_at = frame->push_buffer_base;
             buffer_at < frame->push_buffer_base + frame->push_buffer_used;)
        {
            Render_Group *group = (Render_Group *)buffer_at;
            buffer_at += sizeof(Render_Group);

            for (u8 *group_at = buffer_at;
                 group_at < group->base + group->used;)
            {
                Render_Entity_Header *entity = (Render_Entity_Header *)group_at;
                group_at += entity->size;
                switch (entity->type)
                {
                    case eRender_Bitmap: 
                    {
                        Render_Bitmap *piece = (Render_Bitmap *)entity;

                        glBindBuffer(GL_ARRAY_BUFFER, gl->vbo);

                        glUniformMatrix4fv(program->mvp, 1, GL_TRUE, &frame->ortho_view_proj.e[0][0]);
                        glUniform4fv(program->color, 1, &piece->color.r);

                        glEnableVertexAttribArray(0);
                        glEnableVertexAttribArray(2);

                        glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(Textured_Vertex), (GLvoid *)offsetof(Textured_Vertex, pos));
                        glVertexAttribPointer(2, 2, GL_FLOAT, false, sizeof(Textured_Vertex), (GLvoid *)offsetof(Textured_Vertex, uv));

                        opengl_bind_texture(gl, piece->bitmap);

                        v3 min = piece->min;
                        v3 max = piece->max;

                        Textured_Vertex v[4];
                        f32 z = (max.z + min.z) * 0.5f;
                        v[0].pos = V3(max.x, min.y, z);
                        v[0].uv  = V2(1, 0);

                        v[1].pos = max;
                        v[1].uv  = V2(1, 1);

                        v[2].pos = min;
                        v[2].uv  = V2(0, 0);

                        v[3].pos = V3(min.x, max.y, z);
                        v[3].uv  = V2(0, 1);

                        for (u32 i = 0; i < 4; ++i) {
                            v4 tmp = frame->ortho_view_proj * V4(v[i].pos, 1);
                            v4 temp = frame->ortho_view_proj * V4(v[i].pos, 1);
                        }

                        glBufferData(GL_ARRAY_BUFFER, array_count(v) * sizeof(*v), v, GL_STATIC_DRAW);
                        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

                        glDisableVertexAttribArray(0);
                        glDisableVertexAttribArray(2);
                    } break;
                }
            }
            buffer_at += group->capacity;
        }
    }



    {
        glBindTexture(GL_TEXTURE_2D, 0);
        glActiveTexture(GL_TEXTURE0);
    }
}


internal void
opengl_init(Opengl *gl)
{
#if __DEVELOPER
    if (glDebugMessageCallbackARB) {
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallbackARB(opengl_debug_callback, 0);
    } else {
        Assert("!glDebugMessageCallbackARB not found.");
    }
#endif

    opengl_compile_shaders(gl);

    gl->white_bitmap.bits_per_channel = 8;
    gl->white_bitmap.channel_count = 4;
    gl->white_bitmap.width   = 4;
    gl->white_bitmap.height  = 4;
    gl->white_bitmap.pitch   = 16;
    gl->white_bitmap.handle  = 0;
    gl->white_bitmap.size    = 64;
    gl->white_bitmap.memory  = &gl->white;
    for (u32 *at = (u32 *)gl->white; at <= &gl->white[3][3]; ++at) {
        *at = 0xffffffff;
    }
    opengl_alloc_texture(gl, &gl->white_bitmap, GL_CLAMP_TO_EDGE);

    { //@Shadowmap
        glGenFramebuffers(1, &gl->shadowmap_fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, gl->shadowmap_fbo);

        glGenTextures(1, &gl->shadowmaps);
        glBindTexture(GL_TEXTURE_2D_ARRAY, gl->shadowmaps);
        glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT32F,
                     SHADOWMAP_RESOLUTION, SHADOWMAP_RESOLUTION,
                     CSM_COUNT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

        v4 border_color[] = {1,1,1,1};
        glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, (GLfloat *)&border_color);

        glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, gl->shadowmaps, 0);
        Assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    {
        s32 maxattachment;
        glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &maxattachment);
        Assert(maxattachment >= 4);
    }

    {
        glGenFramebuffers(1, &gl->fbo);

        glGenVertexArrays(1, &gl->vao);
        glBindVertexArray(gl->vao);

        glGenBuffers(1, &gl->vbo);

        glGenBuffers(1, &gl->instance_vbo);

        glGenBuffers(1, &gl->vio);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gl->vio);
    }
}
