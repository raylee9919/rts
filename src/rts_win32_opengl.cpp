// Copyright Seong Woo Lee. All Rights Reserved.

// Desired OpenGL Version
//
#define RTS_GL_VERSION_MAJOR 4
#define RTS_GL_VERSION_MINOR 6
    
// .h
//
#include "base/rts_base_inc.h"
#include "os/os.h"
#include "rts_font.h"
#include "asset/inc.h"
#include "asset.h"

#include "renderer/rts_renderer.h"
#include "renderer/opengl/gl.h"
#include "third_party/opengl/wglext.h"
#include "rts_win32_renderer.h"

// .cpp
//
#include "base/rts_base_inc.cpp"
#include "os/os.cpp"
#include "renderer/opengl/gl.cpp"


extern "C" __declspec(dllexport)
RENDERER_BEGIN_FRAME(win32_begin_frame)
{
    Render_Commands *result = opengl_frame_begin((Opengl *)renderer, window_size, render_size);
    return result;
}

extern "C" __declspec(dllexport)
RENDERER_END_FRAME(win32_end_frame)
{
    gl_frame_end((Opengl *)platform_renderer, renderer, frame);
    HDC hdc = wglGetCurrentDC();
    assert(hdc && "failed to get current dc.");
    assert(SwapBuffers(hdc) && "failed to swap buffer."); 
}

internal Opengl* win32_init_opengl(HDC dc, umm push_buffer_size, Arena *arena, OS_State *os_init)
{
    os = os_init;

    Opengl *gl = push_struct(arena, Opengl);
    GL_Info *glinfo = push_struct(arena, GL_Info);

    // Get WGL functions to be able to create modern GL context.
    get_wgl_functions(gl);

    // Set pixel format for OpenGL context.
    set_pixel_format(gl, dc);
}

extern "C" __declspec(dllexport)
WIN32_LOAD_RENDERER_ENTRY()
{
    Platform_Renderer *result = (Platform_Renderer *)win32_init_opengl(window_dc, push_buffer_size, renderer_arena, os_init);

    return result;
}
