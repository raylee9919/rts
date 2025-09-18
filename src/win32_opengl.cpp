/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Seong Woo Lee $
   $Notice: (C) Copyright 2025 by Seong Woo Lee. All Rights Reserved. $
   ======================================================================== */

// -------------------------------------
// @Note: [.h]
#include "base/rts_base_inc.h"
#include "os/rts_os.h"
#include "rts_math.h"
#include "rts_asset.h"
#include "rts_input.h"

#include <gl/gl.h>
#include <tchar.h>

#include "renderer/rts_renderer.h"
#include "win32_renderer.h"

// -------------------------------------
// @Note: [.cpp]
#include "base/rts_base_inc.cpp"
#include "rts_math.cpp"


#pragma comment(lib, "user32")
#pragma comment(lib, "gdi32")
#pragma comment(lib, "opengl32")



#define WGL_CONTEXT_MAJOR_VERSION_ARB               0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB               0x2092
#define WGL_CONTEXT_LAYER_PLANE_ARB                 0x2093
#define WGL_CONTEXT_FLAGS_ARB                       0x2094
#define WGL_CONTEXT_PROFILE_MASK_ARB                0x9126
#define WGL_CONTEXT_DEBUG_BIT_ARB                   0x0001
#define WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB      0x0002
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB            0x00000001
#define WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB   0x00000002
#define ERROR_INVALID_VERSION_ARB                   0x2095
#define ERROR_INVALID_PROFILE_ARB                   0x2096
#define WGL_NUMBER_PIXEL_FORMATS_ARB                0x2000
#define WGL_DRAW_TO_WINDOW_ARB                      0x2001
#define WGL_DRAW_TO_BITMAP_ARB                      0x2002
#define WGL_ACCELERATION_ARB                        0x2003
#define WGL_NEED_PALETTE_ARB                        0x2004
#define WGL_NEED_SYSTEM_PALETTE_ARB                 0x2005
#define WGL_SWAP_LAYER_BUFFERS_ARB                  0x2006
#define WGL_SWAP_METHOD_ARB                         0x2007
#define WGL_NUMBER_OVERLAYS_ARB                     0x2008
#define WGL_NUMBER_UNDERLAYS_ARB                    0x2009
#define WGL_TRANSPARENT_ARB                         0x200A
#define WGL_TRANSPARENT_RED_VALUE_ARB               0x2037
#define WGL_TRANSPARENT_GREEN_VALUE_ARB             0x2038
#define WGL_TRANSPARENT_BLUE_VALUE_ARB              0x2039
#define WGL_TRANSPARENT_ALPHA_VALUE_ARB             0x203A
#define WGL_TRANSPARENT_INDEX_VALUE_ARB             0x203B
#define WGL_SHARE_DEPTH_ARB                         0x200C
#define WGL_SHARE_STENCIL_ARB                       0x200D
#define WGL_SHARE_ACCUM_ARB                         0x200E
#define WGL_SUPPORT_GDI_ARB                         0x200F
#define WGL_SUPPORT_OPENGL_ARB                      0x2010
#define WGL_DOUBLE_BUFFER_ARB                       0x2011
#define WGL_STEREO_ARB                              0x2012
#define WGL_PIXEL_TYPE_ARB                          0x2013
#define WGL_COLOR_BITS_ARB                          0x2014
#define WGL_RED_BITS_ARB                            0x2015
#define WGL_RED_SHIFT_ARB                           0x2016
#define WGL_GREEN_BITS_ARB                          0x2017
#define WGL_GREEN_SHIFT_ARB                         0x2018
#define WGL_BLUE_BITS_ARB                           0x2019
#define WGL_BLUE_SHIFT_ARB                          0x201A
#define WGL_ALPHA_BITS_ARB                          0x201B
#define WGL_ALPHA_SHIFT_ARB                         0x201C
#define WGL_ACCUM_BITS_ARB                          0x201D
#define WGL_ACCUM_RED_BITS_ARB                      0x201E
#define WGL_ACCUM_GREEN_BITS_ARB                    0x201F
#define WGL_ACCUM_BLUE_BITS_ARB                     0x2020
#define WGL_ACCUM_ALPHA_BITS_ARB                    0x2021
#define WGL_DEPTH_BITS_ARB                          0x2022
#define WGL_STENCIL_BITS_ARB                        0x2023
#define WGL_AUX_BUFFERS_ARB                         0x2024
#define WGL_NO_ACCELERATION_ARB                     0x2025
#define WGL_GENERIC_ACCELERATION_ARB                0x2026
#define WGL_FULL_ACCELERATION_ARB                   0x2027
#define WGL_SWAP_EXCHANGE_ARB                       0x2028
#define WGL_SWAP_COPY_ARB                           0x2029
#define WGL_SWAP_UNDEFINED_ARB                      0x202A
#define WGL_TYPE_RGBA_ARB                           0x202B
#define WGL_TYPE_COLORINDEX_ARB                     0x202C
#define WGL_SAMPLE_BUFFERS_ARB                      0x2041
#define WGL_SAMPLES_ARB                             0x2042
#define WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB            0x20A9



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

global int win32_opengl_attribs[] =
{
    WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
    WGL_CONTEXT_MINOR_VERSION_ARB, 2,
    WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB
#if BUILD_DEBUG
        |WGL_CONTEXT_DEBUG_BIT_ARB
#endif
        ,
#if 0
    WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB,
#else
    WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
#endif
    0,
};

#include "renderer_opengl.h"
#include "renderer_opengl.cpp"

#define WGL_GET_PROC_ADDRESS(Name) Name = (Type_##Name *)wglGetProcAddress(#Name)

#if 0
internal void *
win32_renderer_alloc(umm size)
{
    void *result = VirtualAlloc(0, size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
    return result;
}

internal void
win32_renderer_free(void *memory)
{
    if (memory) {
        VirtualFree(memory, 0, MEM_RELEASE);
    }
}
#endif


internal void
win32_set_pixel_format(Opengl *gl, HDC window_dc) 
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
        
        if (!gl->supports_srgb_framebuffer) {
            int_attrib_list[11] = 0;
        }
        
        wglChoosePixelFormatARB(window_dc, int_attrib_list, 0, 1,
                                &suggested_pixel_format_index, &extended_pick);
    }
    
    if (!extended_pick)
    {
        // @TODO: Hey Raymond Chen - what's the deal here?
        // Is cColorBits ACTUALLY supposed to exclude the alpha bits, like MSDN says, or not?
        PIXELFORMATDESCRIPTOR desired_pixel_format = {};
        desired_pixel_format.nSize      = sizeof(desired_pixel_format);
        desired_pixel_format.nVersion   = 1;
        desired_pixel_format.iPixelType = PFD_TYPE_RGBA;
        desired_pixel_format.dwFlags    = PFD_SUPPORT_OPENGL|PFD_DRAW_TO_WINDOW|PFD_DOUBLEBUFFER;
        desired_pixel_format.cColorBits = 32;
        desired_pixel_format.cAlphaBits = 8;
        desired_pixel_format.cDepthBits = 24;
        desired_pixel_format.iLayerType = PFD_MAIN_PLANE;
        
        suggested_pixel_format_index = ChoosePixelFormat(window_dc, &desired_pixel_format);
    }
    
    PIXELFORMATDESCRIPTOR suggested_pixel_format;
    // @NOTE: Technically you do not need to call DescribePixelFormat here,
    // as SetPixelFormat doesn't actually need it to be filled out properly.
    //DescribePixelFormat(window_dc, suggested_pixel_format_index,
                        //sizeof(suggested_pixel_format), &suggested_pixel_format);
    SetPixelFormat(window_dc, suggested_pixel_format_index, &suggested_pixel_format);
}

internal void
win32_load_wgl_extensions(Opengl *gl) 
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
                        gl->supports_srgb_framebuffer = true;
                    }
                    else if (string_equal(at, count, "WGL_ARB_framebuffer_sRGB")) {
                        gl->supports_srgb_framebuffer = true;
                    }

                    at = end;
                }
            } else {
                Assert(0);
            }

            Assert(wglMakeCurrent(0, 0));
        } else {
            Assert(0);
        }

        Assert(wglDeleteContext(glrc));
        ReleaseDC(window, dc);
        DestroyWindow(window);
        UnregisterClassA(wclass.lpszClassName, wclass.hInstance);
    } else {
        Assert(0);
    }
}

internal void
platform_opengl_set_vsync(Opengl *gl, b32 vsync_enabled)
{
    if(!wglSwapIntervalEXT) {
        wglSwapIntervalEXT = (Wgl_Swap_Interval_Ext *)wglGetProcAddress("wglSwapIntervalEXT");
    }

    if(wglSwapIntervalEXT) {
        wglSwapIntervalEXT(vsync_enabled ? 1 : 0);
    }
}

RENDERER_BEGIN_FRAME(win32_begin_frame)
{
    Render_Commands *result = opengl_frame_begin((Opengl *)renderer, os_window_dim, render_dim);
    return result;
}

RENDERER_END_FRAME(win32_end_frame)
{
    opengl_frame_end((Opengl *)renderer, frame);
    HDC hdc = wglGetCurrentDC();
    if (hdc) 
    {
        if (! SwapBuffers(hdc)) 
        { Assert(0); }
    }
    else 
    { Assert(0); }
}

internal void
win32_get_gl_functions(Opengl_Info info)
{
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
    WGL_GET_PROC_ADDRESS(glBindTextureUnit);
    WGL_GET_PROC_ADDRESS(glGetStringi);
    WGL_GET_PROC_ADDRESS(glGenerateTextureMipmap);
    WGL_GET_PROC_ADDRESS(glPatchParameteri);
    WGL_GET_PROC_ADDRESS(glDeleteShader);
    WGL_GET_PROC_ADDRESS(glFramebufferTexture);
    WGL_GET_PROC_ADDRESS(glTexImage3D);
    WGL_GET_PROC_ADDRESS(glCheckFramebufferStatus);
    WGL_GET_PROC_ADDRESS(glUniform4f);
    WGL_GET_PROC_ADDRESS(glUniform2f);

    if (info.opengl_arb_framebuffer_object) 
    {
        WGL_GET_PROC_ADDRESS(glGenFramebuffers);
        WGL_GET_PROC_ADDRESS(glBindFramebuffer);
        WGL_GET_PROC_ADDRESS(glFramebufferTexture2D);
    }
}

internal Opengl *
win32_init_opengl(HDC window_dc, umm push_buffer_size, Arena *arena, OS os_init)
{
    os = os_init;

    b32 reload = false;

    // @Fix: broke
    // if (arena->used) 
    // { reload = true; }


    Opengl *gl = push_struct(arena, Opengl);
    Opengl_Info *glinfo = push_struct(arena, Opengl_Info);

    if (reload) 
    {
        Opengl *oldgl = push_struct(arena, Opengl);
        gl->info = oldgl->info;
        gl->render_commands = oldgl->render_commands;

        gl->supports_srgb_framebuffer = oldgl->supports_srgb_framebuffer;
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
        Assert(glrc);

        if (wglMakeCurrent(window_dc, glrc)) 
        {
            Opengl_Info info = opengl_get_info(gl, modern_context);
            win32_get_gl_functions(info);
            gl->info = info;

            platform_opengl_set_vsync(gl, true);
        }
        else 
        {
            Assert(0);
        }
    }

    opengl_init(gl);

    return gl;
}

WIN32_LOAD_RENDERER_ENTRY()
{
    Platform_Renderer *result = (Platform_Renderer *)win32_init_opengl(window_dc, push_buffer_size, renderer_arena, os_init);
    
    return result;
}
