// Copyright Seong Woo Lee. All Rights Reserved.

#if OS_WINDOWS
#  include "./rhi_gl_win32.cpp"
#else 
#  error UNDEFINED_OS
#endif


// @Temporary
#include "renderer/opengl/gl.cpp"


internal void r_begin(RHI_State* rhi, v2 window_size, v2 render_size) {
    Opengl* gl = (Opengl*)rhi->platform;
    gl_begin(gl, window_size, render_size);
}

internal void r_end(RHI_State* rhi, Renderer* r) {
    Opengl* gl = (Opengl*)rhi->platform;
    gl_end(gl, r);

    // Swap
    HDC hdc = wglGetCurrentDC();
    assert(hdc && "failed to get current dc.");
    assert(SwapBuffers(hdc) && "failed to swap buffer."); 
}
