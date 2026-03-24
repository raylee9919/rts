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
typedef HGLRC WINAPI Wgl_Create_Context_Attribs_Arb(HDC hDC, HGLRC hShareContext, const int *attribList);
typedef BOOL WINAPI Wgl_Get_Pixel_Format_Attrib_Iv_Arb(HDC hdc, int iPixelFormat, int iLayerPlane, UINT nAttributes, const int *piAttributes, int *piValues);
typedef BOOL WINAPI Wgl_Get_Pixel_Format_Attrib_Fv_Arb(HDC hdc, int iPixelFormat, int iLayerPlane, UINT nAttributes, const int *piAttributes, FLOAT *pfValues);
typedef BOOL WINAPI Wgl_Choose_Pixel_Format_Arb(HDC hdc, const int *piAttribIList, const FLOAT *pfAttribFList, UINT nMaxFormats, int *piFormats, UINT *n);
typedef BOOL WINAPI Wgl_Swap_Interval_Ext(int interval);
typedef const char * WINAPI Wgl_Get_Extensions_String_Ext(void);

global Wgl_Create_Context_Attribs_Arb *wglCreateContextAttribsARB;
global Wgl_Choose_Pixel_Format_Arb *wglChoosePixelFormatARB;
global Wgl_Swap_Interval_Ext *wglSwapIntervalEXT;
global Wgl_Get_Extensions_String_Ext *wglGetExtensionsStringEXT;

// WGL Globals.
//
global int win32_opengl_attribs[] =
{
    WGL_CONTEXT_MAJOR_VERSION_ARB, RTS_GL_VERSION_MAJOR,
    WGL_CONTEXT_MINOR_VERSION_ARB, RTS_GL_VERSION_MINOR,
    WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
    WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB
#if BUILD_DEBUG
        |WGL_CONTEXT_DEBUG_BIT_ARB
#endif
        ,
    0,
};

internal void win32_set_pixel_format(Opengl *gl, HDC window_dc) 
{
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
}

internal void win32_load_wgl_extensions(Opengl *gl) 
{
    WNDCLASSA wclass = {};
    wclass.lpfnWndProc   = DefWindowProcA;
    wclass.hInstance     = GetModuleHandle(0);
    wclass.lpszClassName = "WGL_Loader";

    if (RegisterClassA(&wclass)) 
    {
        HWND window = CreateWindowExA(0, wclass.lpszClassName, "WGL_Loader", 0, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, wclass.hInstance, 0);
        HDC dc = GetDC(window);
        win32_set_pixel_format(gl, dc);
        HGLRC glrc = wglCreateContext(dc);
        if (wglMakeCurrent(dc, glrc)) 
        {
            wglChoosePixelFormatARB    = (Wgl_Choose_Pixel_Format_Arb *)wglGetProcAddress("wglChoosePixelFormatARB");
            wglCreateContextAttribsARB = (Wgl_Create_Context_Attribs_Arb *)wglGetProcAddress("wglCreateContextAttribsARB");
            wglGetExtensionsStringEXT  = (Wgl_Get_Extensions_String_Ext *)wglGetProcAddress("wglGetExtensionsStringEXT");

            if (wglGetExtensionsStringEXT)
            {
                char *extensions = (char *)wglGetExtensionsStringEXT();
                char *at = extensions;
                while (*at)
                {
                    while (is_whitespace(*at)) { ++at; }
                    char *end = at;
                    while (*end && !is_whitespace(*end)) {++end;}

                    umm count = end - at;

                    if (string_equal(at, count, "WGL_EXT_framebuffer_sRGB")) {
                        gl->info.ARB_EXT_framebuffer_srgb = true;
                    }
                    else if (string_equal(at, count, "WGL_ARB_framebuffer_sRGB")) {
                        gl->info.ARB_EXT_framebuffer_srgb = true;
                    }

                    at = end;
                }
            } else {
                assert(0);
            }

            assert(wglMakeCurrent(0, 0));
        } else {
            assert(0);
        }

        assert(wglDeleteContext(glrc));
        ReleaseDC(window, dc);
        DestroyWindow(window);
        UnregisterClassA(wclass.lpszClassName, wclass.hInstance);
    } else {
        assert(0);
    }
}

internal void
platform_opengl_set_vsync(Opengl *gl, b32 vsync_enabled)
{
    if(! wglSwapIntervalEXT) 
    {
        wglSwapIntervalEXT = (Wgl_Swap_Interval_Ext *)wglGetProcAddress("wglSwapIntervalEXT");
    }

    if (wglSwapIntervalEXT) 
    {
        wglSwapIntervalEXT(vsync_enabled ? 1 : 0);
    }
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
    WGL_GET_PROC_ADDRESS(glCreateShader);
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
    WGL_GET_PROC_ADDRESS(glTexStorage3D);
    WGL_GET_PROC_ADDRESS(glTexSubImage3D);
    WGL_GET_PROC_ADDRESS(glGenerateMipmap);
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

    if (info.opengl_arb_framebuffer_object) 
    {
        WGL_GET_PROC_ADDRESS(glGenFramebuffers);
        WGL_GET_PROC_ADDRESS(glBindFramebuffer);
        WGL_GET_PROC_ADDRESS(glFramebufferTexture2D);
    }
}

internal Opengl* win32_init_opengl(HDC window_dc, umm push_buffer_size, Arena *arena, OS *os_init)
{
    os = os_init;

    b32 reload = false;

    // @Fix: broke
    // if (arena->used) 
    // { reload = true; }


    Opengl *gl = push_struct(arena, Opengl);
    GL_Info *glinfo = push_struct(arena, GL_Info);

    if (reload) 
    {
        Opengl *oldgl = push_struct(arena, Opengl);
        gl->info = oldgl->info;
        gl->render_commands = oldgl->render_commands;

        gl->info.ARB_EXT_framebuffer_srgb = oldgl->info.ARB_EXT_framebuffer_srgb;
        HGLRC glrc = wglGetCurrentContext();
        win32_load_wgl_extensions(gl);
        wglMakeCurrent(window_dc, glrc);
    }
    else 
    {
        win32_set_pixel_format(gl, window_dc);
        win32_load_wgl_extensions(gl);
    }

    gl->push_buffer = (u8 *)push_size(arena, push_buffer_size);
    gl->push_buffer_size = push_buffer_size;

    if (reload) 
    {
        win32_get_gl_functions(gl->info);
    } 
    else 
    {
        b32 modern_context = true;
        HGLRC glrc = 0;
        if (wglCreateContextAttribsARB) 
        {
            glrc = wglCreateContextAttribsARB(window_dc, 0, win32_opengl_attribs);
        }
        if (! glrc) 
        {
            modern_context = false;
            glrc = wglCreateContext(window_dc);
        }
        assert(glrc);

        if (wglMakeCurrent(window_dc, glrc)) 
        {
#define X(type, name) name = (type)wglGetProcAddress(#name); assert(name);
        GL_FUNCTIONS(X)
#undef X
            GL_Info info = opengl_get_info(gl, modern_context);
            win32_get_gl_functions(info);
            gl->info = info;

            platform_opengl_set_vsync(gl, true);
        }
        else 
        {
            assert(0);
        }
    }

    gl_init(gl);

    return gl;
}

extern "C" __declspec(dllexport)
WIN32_LOAD_RENDERER_ENTRY()
{
    Platform_Renderer *result = (Platform_Renderer *)win32_init_opengl(window_dc, push_buffer_size, renderer_arena, os_init);
    
    return result;
}
