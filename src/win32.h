/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Seong Woo Lee $
   $Notice: (C) Copyright %s by Seong Woo Lee. All Rights Reserved. $
   ======================================================================== */

struct Win32_Dimension 
{
    int width;
    int height;
};

#define WIN32_MAX_PATH_LENGTH MAX_PATH
struct Win32_State 
{
    HWND default_window_handle;

    char exe_filepath[WIN32_MAX_PATH_LENGTH];
    char *one_past_last_exe_filepath_slash;

    HANDLE      record_file;
    b32         is_recording;
    b32         is_playing;

    void        *game_memory;
    umm         game_memory_total_size;
};

struct Win32_Loaded_Code
{
    b32 is_valid;
    u32 temp_dll_name;
    
    char *transient_dll_name;
    char *dll_full_path;
    char *lock_full_path;
    
    HMODULE dll;
    FILETIME dll_last_write_time;
    
    u32 function_count;
    char **function_names;
    void **functions;
};

struct Win32_Game_Function_Table
{
    Game_Update_And_Render *update_and_render;
};

global char *win32_game_function_table_names[] =
{
    "game_update_and_render"
};




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
