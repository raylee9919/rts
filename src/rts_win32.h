/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Seong Woo Lee $
   $Notice: (C) Copyright %s by Seong Woo Lee. All Rights Reserved. $
   ======================================================================== */


#define WIN32_MAX_PATH_LENGTH MAX_PATH
struct Win32_State 
{
    Arena *arena;
    HWND main_hwnd;

    Utf8 binary_path;
    Utf8 game_dll_path;
    Utf8 renderer_dll_path;
    Utf8 lock_path;
};

struct Win32_Code
{
    b32 is_valid;

    Utf8 temp_dll_path;
    Utf8 dll_path;
    Utf8 lock_path;

    HMODULE dll;
    u64 last_modified;

    u32 function_count;
    char **function_names;
    void **functions;
};

struct Win32_Game_Function_Table
{
    Game_Update_And_Render *update_and_render;
};

global read_only char *win32_game_function_table_names[] =
{
    "game_update_and_render"
};


// ----------------------------------------------------
// @Note: Window
internal LRESULT win32_window_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
internal HWND win32_create_window(HINSTANCE hinst);
internal void win32_toggle_fullscreen(HWND window);
internal v2u win32_client_size_from_hwnd(HWND hwnd);




// ----------------------------------------------------
// @Note: OpenGL
#define WGL_GET_PROC_ADDRESS(Name) Name = (Type_##Name *)wglGetProcAddress(#Name)

typedef HGLRC Type_wglCreateContextAttribsARB(HDC hDC, HGLRC hShareContext, const int *attribList);
global Type_wglCreateContextAttribsARB *wglCreateContextAttribsARB;


typedef BOOL Type_wglChoosePixelFormatARB(HDC hdc,
                                          const int *piAttribIList,
                                          const FLOAT *pfAttribFList,
                                          UINT nMaxFormats,
                                          int *piFormats,
                                          UINT *nNumFormats);
global Type_wglChoosePixelFormatARB *wglChoosePixelFormatARB;
