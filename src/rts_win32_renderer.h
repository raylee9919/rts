#ifndef RTS_WIN32_RENDERER_H
#define RTS_WIN32_RENDERER_H
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Seong Woo Lee $
   $Notice: (C) Copyright %s by Seong Woo Lee. All Rights Reserved. $
   ======================================================================== */





// ----------------------------------------------------
// @Note: Renderer
#define WIN32_LOAD_RENDERER(name) Platform_Renderer *name(HDC window_dc, u64 push_buffer_size, struct Arena *renderer_arena, OS os_init)
typedef WIN32_LOAD_RENDERER(Win32_Load_Renderer);
#define WIN32_LOAD_RENDERER_ENTRY() WIN32_LOAD_RENDERER(win32_load_renderer)

struct Win32_Renderer_Function_Table
{
    Win32_Load_Renderer *load_renderer;
    Renderer_Begin_Frame *begin_frame;
    Renderer_End_Frame *end_frame;
};

global char *win32_renderer_function_table_names[] = 
{
    "win32_load_renderer",
    "win32_begin_frame",
    "win32_end_frame",
};









#endif // RTS_WIN32_RENDERER_H
