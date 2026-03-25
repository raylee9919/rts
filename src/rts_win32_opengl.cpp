// Copyright Seong Woo Lee. All Rights Reserved.

// Desired OpenGL Version
//
#define RTS_GL_VERSION_MAJOR 4
#define RTS_GL_VERSION_MINOR 5
    
// .h
//
#include "base/rts_base_inc.h"
#include "os/rts_os.h"
#include "rts_font.h"
#include "asset/texture.h"
#include "asset.h"

#include "renderer/rts_renderer.h"
#include "renderer/opengl/gl.h"
#include "third_party/opengl/wglext.h"
#include "rts_win32_renderer.h"

// .cpp
//
#include "base/rts_base_inc.cpp"
#include "renderer/opengl/gl.cpp"


// Windows-specific directives.
//
#pragma comment(lib, "user32")
#pragma comment(lib, "gdi32")
#pragma comment(lib, "opengl32")


// WGL Functions.
//
typedef BOOL WINAPI Wgl_Get_Pixel_Format_Attrib_Iv_Arb(HDC hdc, int iPixelFormat, int iLayerPlane, UINT nAttributes, const int *piAttributes, int *piValues);
typedef BOOL WINAPI Wgl_Get_Pixel_Format_Attrib_Fv_Arb(HDC hdc, int iPixelFormat, int iLayerPlane, UINT nAttributes, const int *piAttributes, FLOAT *pfValues);

static PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB = NULL;
static PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB = NULL;
static PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT = NULL;


internal void set_pixel_format(Opengl *gl, HDC dc) 
{
    int attrib[] =
    {
        WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
        WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
        WGL_DOUBLE_BUFFER_ARB,  GL_TRUE,
        WGL_PIXEL_TYPE_ARB,     WGL_TYPE_RGBA_ARB,
        WGL_COLOR_BITS_ARB,     24,
        WGL_DEPTH_BITS_ARB,     24,
        WGL_STENCIL_BITS_ARB,   8,

        // uncomment for sRGB framebuffer, from WGL_ARB_framebuffer_sRGB extension
        // https://www.khronos.org/registry/OpenGL/extensions/ARB/ARB_framebuffer_sRGB.txt
        WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB, GL_TRUE,

        // uncomment for multisampled framebuffer, from WGL_ARB_multisample extension
        // https://www.khronos.org/registry/OpenGL/extensions/ARB/ARB_multisample.txt
        //WGL_SAMPLE_BUFFERS_ARB, 1,
        //WGL_SAMPLES_ARB,        4, // 4x MSAA

        0,
    };

    int format;
    UINT formats;
    if (!wglChoosePixelFormatARB(dc, attrib, NULL, 1, &format, &formats) || formats == 0) {
        assert(!"OpenGL does not support required pixel format!");
    }

    PIXELFORMATDESCRIPTOR desc = {};
    desc.nSize = sizeof(desc);
    int ok = DescribePixelFormat(dc, format, sizeof(desc), &desc);
    assert(ok && "Failed to describe OpenGL pixel format");

    if (!SetPixelFormat(dc, format, &desc)) {
        assert(!"Cannot set OpenGL selected pixel format!");
    }



#if 0
    int suggested_pixel_format_index = 0;
    GLuint extended_pick = 0;
    if (wglChoosePixelFormatARB)
    {
        int int_attrib_list[] =
        {
            WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,                    // 0
            WGL_ACCELERATION_ARB, WGL_FULL_ACCELERATION_ARB,    // 1
            WGL_SUPPORT_OPENGL_ARB, GL_TRUE,                    // 2
            WGL_DOUBLE_BUFFER_ARB, GL_TRUE,                     // 3
            WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,              // 4
            WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB, GL_TRUE,          // 5
            0,
        };

        if (!gl->info.ARB_EXT_framebuffer_srgb) {
            int_attrib_list[11] = 0;
        }

        wglChoosePixelFormatARB(window_dc, int_attrib_list, 0, 1,
                                &suggested_pixel_format_index, &extended_pick);
    }

    if (! extended_pick)
    {
        // # Todo: Hey Raymond Chen - what's the deal here?
        //         Is cColorBits ACTUALLY supposed to exclude the alpha bits, like MSDN says, or not?
        PIXELFORMATDESCRIPTOR desired_pixel_format = {};
        {
            desired_pixel_format.nSize      = sizeof(desired_pixel_format);
            desired_pixel_format.nVersion   = 1;
            desired_pixel_format.iPixelType = PFD_TYPE_RGBA;
            desired_pixel_format.dwFlags    = PFD_SUPPORT_OPENGL|PFD_DRAW_TO_WINDOW|PFD_DOUBLEBUFFER;
            desired_pixel_format.cColorBits = 32;
            desired_pixel_format.cAlphaBits = 8;
            desired_pixel_format.cDepthBits = 24;
            desired_pixel_format.iLayerType = PFD_MAIN_PLANE;
        }

        suggested_pixel_format_index = ChoosePixelFormat(window_dc, &desired_pixel_format);
    }

    PIXELFORMATDESCRIPTOR suggested_pixel_format;
    // Technically you do not need to call DescribePixelFormat here,
    // as SetPixelFormat doesn't actually need it to be filled out properly.
    // DescribePixelFormat(window_dc, suggested_pixel_format_index, sizeof(suggested_pixel_format), &suggested_pixel_format);
    SetPixelFormat(window_dc, suggested_pixel_format_index, &suggested_pixel_format);
#endif
}

internal void get_wgl_functions(Opengl *gl) 
{
    HWND dummy = CreateWindowExW(0, L"STATIC", L"DummyWindow", WS_OVERLAPPED,
                                 CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                                 NULL, NULL, NULL, NULL);

    HDC dc = GetDC(dummy);
    assert(dc && "Failed to get device context for dummy window.");

    PIXELFORMATDESCRIPTOR desc = {};
    desc.nSize = sizeof(desc);
    desc.nVersion = 1;
    desc.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    desc.iPixelType = PFD_TYPE_RGBA;
    desc.cColorBits = 24;

    int format = ChoosePixelFormat(dc, &desc);
    if (!format) {
        assert(!"Cannot choose OpenGL pixel format for dummy window!");
    }

    int ok = DescribePixelFormat(dc, format, sizeof(desc), &desc);
    assert(ok && "Failed to describe OpenGL pixel format");

    // reason to create dummy window is that SetPixelFormat can be called only once for the window
    if (!SetPixelFormat(dc, format, &desc)) {
        assert(!"Cannot set OpenGL pixel format for dummy window!");
    }

    HGLRC rc = wglCreateContext(dc);
    assert(rc && "Failed to create OpenGL context for dummy window.");

    ok = wglMakeCurrent(dc, rc);
    assert(ok && "Failed to make current OpenGL context for dummy window.");

    // https://www.khronos.org/registry/OpenGL/extensions/ARB/WGL_ARB_extensions_string.txt
    auto wglGetExtensionsStringARB = (PFNWGLGETEXTENSIONSSTRINGARBPROC)wglGetProcAddress("wglGetExtensionsStringARB");
    if (!wglGetExtensionsStringARB) {
        assert(!"OpenGL does not support WGL_ARB_extensions_string extension!");
    }

    const char* ext = wglGetExtensionsStringARB(dc);
    assert(ext && "Failed to get OpenGL WGL extension string.");

    char* start = (char *)ext;
    for (;;) {
        while (*ext != 0 && *ext != ' ') {
            ext++;
        }

        size_t length = ext - start;
        if (string_equal("WGL_ARB_pixel_format", start, length)) {
            // https://www.khronos.org/registry/OpenGL/extensions/ARB/WGL_ARB_pixel_format.txt
            wglChoosePixelFormatARB = (PFNWGLCHOOSEPIXELFORMATARBPROC)wglGetProcAddress("wglChoosePixelFormatARB");
        }
        else if (string_equal("WGL_ARB_create_context", start, length)) {
            // https://www.khronos.org/registry/OpenGL/extensions/ARB/WGL_ARB_create_context.txt
            wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");
        }
        else if (string_equal("WGL_EXT_swap_control", start, length)) {
            // https://www.khronos.org/registry/OpenGL/extensions/EXT/WGL_EXT_swap_control.txt
            wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");
        } else if (string_equal("WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB", start, length)) {

        }
 
        if (*ext == 0) {
            break;
        }

        ext++;
        start = (char *)ext;
    }

    if (!wglChoosePixelFormatARB || !wglCreateContextAttribsARB || !wglSwapIntervalEXT) {
        assert(!"OpenGL does not support required WGL extensions for modern context!");
    }


    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(rc);
    ReleaseDC(dummy, dc);
    DestroyWindow(dummy);
}

extern "C" __declspec(dllexport)
RENDERER_BEGIN_FRAME(win32_begin_frame)
{
    Render_Commands *result = opengl_frame_begin((Opengl *)renderer, os_window_dim, render_dim);
    return result;
}

extern "C" __declspec(dllexport)
RENDERER_END_FRAME(win32_end_frame)
{
    gl_frame_end((Opengl *)platform_renderer, renderer, frame);
    HDC hdc = wglGetCurrentDC();
    if (hdc) 
    {
        if (! SwapBuffers(hdc)) 
        {
            assert(0); 
        }
    }
    else 
    {
        assert(0); 
    }
}

internal void win32_get_gl_functions(GL_Info info)
{
#define WGL_GET_PROC_ADDRESS(name) name = (Type_##name *)wglGetProcAddress(#name)
    WGL_GET_PROC_ADDRESS(glShaderSource);
    WGL_GET_PROC_ADDRESS(glCompileShader);
    WGL_GET_PROC_ADDRESS(glCreateProgram);
    WGL_GET_PROC_ADDRESS(glAttachShader);
    WGL_GET_PROC_ADDRESS(glLinkProgram);
    WGL_GET_PROC_ADDRESS(glGetProgramiv);
    WGL_GET_PROC_ADDRESS(glGetShaderInfoLog);
    WGL_GET_PROC_ADDRESS(glValidateProgram);
    WGL_GET_PROC_ADDRESS(glGetProgramInfoLog);
    WGL_GET_PROC_ADDRESS(glGenBuffers);
    WGL_GET_PROC_ADDRESS(glBindBuffer);
    WGL_GET_PROC_ADDRESS(glUniformMatrix4fv);
    WGL_GET_PROC_ADDRESS(glGetUniformLocation);
    WGL_GET_PROC_ADDRESS(glUseProgram);
    WGL_GET_PROC_ADDRESS(glUniform1i);
    WGL_GET_PROC_ADDRESS(glBufferData);
    WGL_GET_PROC_ADDRESS(glVertexAttribPointer);
    WGL_GET_PROC_ADDRESS(glGetAttribLocation);
    WGL_GET_PROC_ADDRESS(glEnableVertexAttribArray);
    WGL_GET_PROC_ADDRESS(glGenVertexArrays);
    WGL_GET_PROC_ADDRESS(glBindVertexArray);
    WGL_GET_PROC_ADDRESS(glBindAttribLocation);
    WGL_GET_PROC_ADDRESS(glDebugMessageCallbackARB);
    WGL_GET_PROC_ADDRESS(glDisableVertexAttribArray);
    WGL_GET_PROC_ADDRESS(glUniform3fv);
    WGL_GET_PROC_ADDRESS(glVertexAttribIPointer);
    WGL_GET_PROC_ADDRESS(glUniform4fv);
    WGL_GET_PROC_ADDRESS(glVertexAttribDivisor);
    WGL_GET_PROC_ADDRESS(glDrawElementsInstanced);
    WGL_GET_PROC_ADDRESS(glUniform1f);
    WGL_GET_PROC_ADDRESS(glUniform1fv);
    WGL_GET_PROC_ADDRESS(glTexSubImage3D);
    WGL_GET_PROC_ADDRESS(glBindImageTexture);
    WGL_GET_PROC_ADDRESS(glClearTexImage);
    WGL_GET_PROC_ADDRESS(glDrawBuffers);
    WGL_GET_PROC_ADDRESS(glActiveTexture);
    WGL_GET_PROC_ADDRESS(glBindRenderbuffer);
    WGL_GET_PROC_ADDRESS(glRenderbufferStorage);
    WGL_GET_PROC_ADDRESS(glFramebufferRenderbuffer);
    WGL_GET_PROC_ADDRESS(glGenRenderbuffers);
    WGL_GET_PROC_ADDRESS(glBufferSubData);
    WGL_GET_PROC_ADDRESS(glBufferStorage);
    WGL_GET_PROC_ADDRESS(glBindBufferBase);
    WGL_GET_PROC_ADDRESS(glGetBufferSubData);
    WGL_GET_PROC_ADDRESS(glTexBuffer);
    WGL_GET_PROC_ADDRESS(glUniform1ui);
    WGL_GET_PROC_ADDRESS(glDispatchCompute);
    WGL_GET_PROC_ADDRESS(glMemoryBarrier);
    WGL_GET_PROC_ADDRESS(glMapBufferRange);
    WGL_GET_PROC_ADDRESS(glUnmapBuffer);
    WGL_GET_PROC_ADDRESS(glGetIntegeri_v);
    WGL_GET_PROC_ADDRESS(glDeleteBuffers);
    WGL_GET_PROC_ADDRESS(glClearNamedBufferData);
    WGL_GET_PROC_ADDRESS(glDeleteFramebuffers);
    WGL_GET_PROC_ADDRESS(glDeleteRenderbuffers);
    WGL_GET_PROC_ADDRESS(glGenerateTextureMipmap);
    WGL_GET_PROC_ADDRESS(glPatchParameteri);
    WGL_GET_PROC_ADDRESS(glDeleteShader);
    WGL_GET_PROC_ADDRESS(glFramebufferTexture);
    WGL_GET_PROC_ADDRESS(glTexImage3D);
    WGL_GET_PROC_ADDRESS(glCheckFramebufferStatus);
    WGL_GET_PROC_ADDRESS(glUniform4f);
    WGL_GET_PROC_ADDRESS(glUniform2f);
    WGL_GET_PROC_ADDRESS(glGetUniformiv);
    WGL_GET_PROC_ADDRESS(glGetActiveUniform);
    WGL_GET_PROC_ADDRESS(glGetActiveAttrib);
    WGL_GET_PROC_ADDRESS(glDrawArraysInstanced);
    WGL_GET_PROC_ADDRESS(glGetShaderiv);
    WGL_GET_PROC_ADDRESS(glCreateTextures);
    WGL_GET_PROC_ADDRESS(glTextureParameteri);
    WGL_GET_PROC_ADDRESS(glTextureStorage2D);
    WGL_GET_PROC_ADDRESS(glTextureSubImage2D);

    if (info.ARB_framebuffer_object) {
        WGL_GET_PROC_ADDRESS(glBindFramebuffer);
        WGL_GET_PROC_ADDRESS(glFramebufferTexture2D);
    }
}

internal Opengl* win32_init_opengl(HDC dc, umm push_buffer_size, Arena *arena, OS *os_init)
{
    os = os_init;

    Opengl *gl = push_struct(arena, Opengl);
    GL_Info *glinfo = push_struct(arena, GL_Info);

    // Get WGL functions to be able to create modern GL context.
    get_wgl_functions(gl);

    // Set pixel format for OpenGL context.
    set_pixel_format(gl, dc);


    int attrib[] =
    {
        WGL_CONTEXT_MAJOR_VERSION_ARB, RTS_GL_VERSION_MAJOR,
        WGL_CONTEXT_MINOR_VERSION_ARB, RTS_GL_VERSION_MINOR,
        WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
#if BUILD_DEBUG
        // ask for debug context for non "Release" builds
        // this is so we can enable debug callback
        WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_DEBUG_BIT_ARB,
#endif
        0,
    };

    HGLRC rc = wglCreateContextAttribsARB(dc, NULL, attrib);
    if (!rc) {
        assert(!"Cannot create modern OpenGL context! OpenGL version 4.5 not supported?");
    }

    BOOL ok = wglMakeCurrent(dc, rc);
    assert(ok && "Failed to make current OpenGL Context.");


#define X(type, name) name = (type)wglGetProcAddress(#name); assert(name);
    GL_FUNCTIONS(X);
#undef X

    // V-Sync
    wglSwapIntervalEXT(true);


    // @Todo: wtf is this..
    gl->push_buffer = (u8 *)push_size(arena, push_buffer_size);
    gl->push_buffer_size = push_buffer_size;


    // @Todo: Cleanup
    GL_Info info = opengl_get_info(gl, true);
    win32_get_gl_functions(info);
    gl->info = info;


    // @Todo: Cleanup
    gl_init(gl);
    return gl;
}

extern "C" __declspec(dllexport)
WIN32_LOAD_RENDERER_ENTRY()
{
    Platform_Renderer *result = (Platform_Renderer *)win32_init_opengl(window_dc, push_buffer_size, renderer_arena, os_init);
    
    return result;
}
