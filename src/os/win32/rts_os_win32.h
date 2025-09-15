#ifndef RTS_OS_WIN32_H
#define RTS_OS_WIN32_H
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Seong Woo Lee $
   $Notice: (C) Copyright %s by Seong Woo Lee. All Rights Reserved. $
   ======================================================================== */

#define NOMINMAX
#define UNICODE
#define _UNICODE
#include <windows.h>
#include <shlobj.h>
#include <Xinput.h>
#include <xaudio2.h>

#pragma comment(lib, "user32")
#pragma comment(lib, "gdi32")
#pragma comment(lib, "shell32")
#pragma comment(lib, "winmm")
#pragma comment(lib, "ole32")


struct Win32_File_Find_Data
{
    HANDLE              handle;
    b32                 returned_first;
    WIN32_FIND_DATAW    find_data;
};
static_assert( sizeof(Win32_File_Find_Data) <= sizeof(Os_File_Iterator) );


#endif // RTS_OS_WIN32_H
