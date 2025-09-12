/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Sung Woo Lee $
   $Notice: (C) Copyright 2025 by Sung Woo Lee. All Rights Reserved. $
   ======================================================================== */



    

#define WIN32_LOAD_RENDERER(name) Platform_Renderer *name(HDC window_dc, umm push_buffer_size, struct Arena *renderer_arena, OS os_init)
typedef WIN32_LOAD_RENDERER(Win32_Load_Renderer);
#define WIN32_LOAD_RENDERER_ENTRY() WIN32_LOAD_RENDERER(win32_load_renderer)

struct Win32_Renderer_Function_Table
{
    Win32_Load_Renderer *load_renderer;
    Renderer_Begin_Frame *begin_frame;
    Renderer_End_Frame *end_frame;
    Renderer_Cleanup *cleanup;
};

global char *win32_renderer_function_table_names[] = 
{
    "win32_load_renderer",
    "win32_begin_frame",
    "win32_end_frame",
    "win32_cleanup",
};
