// Copyright Seong Woo Lee. All Rights Reserved.

internal void get_wgl_functions(Opengl *gl) {
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

internal void set_pixel_format(Opengl *gl, HDC dc) {
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
}

internal bool rhi_init(RHI_State* rhi, OS_Handle window) {
    Arena* arena = rhi->arena;

    HWND hwnd = hwnd_from_os_handle(window);
    HDC hdc = GetDC(hwnd);

    Opengl *gl = push_struct(arena, Opengl);
    GL_Info *glinfo = push_struct(arena, GL_Info);

    // Get WGL functions to be able to create modern GL context.
    get_wgl_functions(gl);

    // Set pixel format for OpenGL context.
    set_pixel_format(gl, hdc);

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

    HGLRC rc = wglCreateContextAttribsARB(hdc, NULL, attrib);
    if (!rc) {
        assert(!"Cannot create modern OpenGL context! OpenGL version 4.5 not supported?");
    }

    BOOL ok = wglMakeCurrent(hdc, rc);
    assert(ok && "Failed to make current OpenGL Context.");


#define X(type, name) name = (type)wglGetProcAddress(#name); assert(name);
    GL_FUNCTIONS(X);
#undef X

    // V-Sync
    wglSwapIntervalEXT(true);


    // @Todo: wtf is this..
    u64 push_buffer_size = MB(48);
    gl->push_buffer = (u8 *)push_size(arena, push_buffer_size);
    gl->push_buffer_size = push_buffer_size;


    // @Todo: Cleanup
    GL_Info info = opengl_get_info(gl, true);
    gl->info = info;


    // @Todo: Cleanup
    gl_init(gl);

    rhi->platform = gl;

    return true;
}
