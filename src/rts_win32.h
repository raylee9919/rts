// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RTS_WIN32_H
#define RTS_WIN32_H


#define WIN32_MAX_PATH_LENGTH MAX_PATH
struct Win32_State 
{
    Arena *arena;

    Utf8 binary_path;
    Utf8 renderer_dll_path;
    Utf8 lock_path;
};

struct Win32_Code
{
    b32 is_valid;

    u8 temp_dll_path_prefix;
    Utf8 temp_dll_name;
    Utf8 temp_dll_path;
    Utf8 dll_path;
    Utf8 lock_path;

    HMODULE dll;

    u32 function_count;
    char **function_names;
    void **functions;
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

#endif
