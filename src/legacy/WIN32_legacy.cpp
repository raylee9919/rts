/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Sung Woo Lee $
   $Notice: (C) Copyright 2024 by Sung Woo Lee. All Rights Reserved. $
   ======================================================================== */



/*
 *
 * These were mainly used for software (CPU) rendering.
 *
 */
#if 0

struct Win32ScreenBuffer 
{
    BITMAPINFO bitmap_info;
    void *memory;
    int width;
    int height;
    int bpp;
};

internal void
win32_resize_dib_section(Win32ScreenBuffer *screen_buffer, int width, int height) 
{
#if 0
    if (screen_buffer->memory) 
        VirtualFree(g_screen_buffer.memory, 0, MEM_RELEASE);
#endif

    screen_buffer->width = width;
    screen_buffer->height = height;
    screen_buffer->bpp = 4;

#if 0
    BITMAPINFOHEADER *header = &screen_buffer->bitmap_info.bmiHeader;
    header->biWidth = width;
    header->biHeight = height;
    header->biSize = sizeof(screen_buffer->bitmap_info.bmiHeader);
    header->biWidth = width;
    header->biHeight = -height;
    header->biPlanes = 1;
    header->biBitCount = 32;
    header->biCompression = BI_RGB;

    screen_buffer->memory = VirtualAlloc(0, width * height * screen_buffer->bpp,
                                         MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
#endif
}
#endif

#if 0
internal void 
win32_update_screen(HDC hdc, int windowWidth, int windowHeight) 
{
    // NOTE: For debugging purpose
    StretchDIBits(hdc,
#if 0
                  0, 0, g_screen_buffer.width, g_screen_buffer.height,
#else
                  0, 0, windowWidth, windowHeight,
#endif
                  0, 0, g_screen_buffer.width, g_screen_buffer.height,
                  g_screen_buffer.memory, &g_screen_buffer.bitmap_info, DIB_RGB_COLORS, SRCCOPY);
}

#endif

