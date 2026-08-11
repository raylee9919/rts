// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RTS_OS_WIN32_H
#define RTS_OS_WIN32_H

#define COM_SAFE_RELEASE(ppT) if (*(ppT)) { (*(ppT))->Release(); *(ppT) = NULL; }

#define NOMINMAX
#define UNICODE
#define _UNICODE
#include <windows.h>
#include <windowsx.h>
#include <shlobj.h>
#include <Xinput.h>
#include <psapi.h>
#include <uxtheme.h>
#include <vssym32.h>

#pragma comment(lib, "user32")
#pragma comment(lib, "gdi32")
#pragma comment(lib, "uxtheme")
#pragma comment(lib, "shell32")
#pragma comment(lib, "winmm")
#pragma comment(lib, "ole32")

extern "C" 
{
    __declspec(dllexport) DWORD NvOptimusEnablement = 1;
    __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}

// DXGI
#pragma comment(lib, "dxgi")
#pragma comment(lib, "dxguid")
#include <dxgi1_6.h>
#include <dxgidebug.h>

// D3D12
#pragma comment(lib, "d3d12")
#include <ThirdParty/DirectX/Include/d3d12.h>
#include <ThirdParty/DirectX/Include/d3d12shader.h>
#include <ThirdParty/DirectX/Include/d3dx12/d3dx12.h>

// DXC
#pragma comment(lib, "dxcompiler")
#include <ThirdParty/DXC/Include/dxcapi.h>
#include <ThirdParty/DirectX/Include/d3d12compiler.h>

// PIX
#if BUILD_PROFILE
# define USE_PIX 1
# pragma comment(lib, "WinPixEventRuntime")
#endif

typedef CRITICAL_SECTION Critical_Section;


struct Win32_Window {
    Win32_Window *next;
    Win32_Window *prev;
    HWND handle;
    WINDOWPLACEMENT placement;
};

struct Win32_State {
    Arena *window_arena;
    Win32_Window *window_first;
    Win32_Window *window_last;
    Win32_Window *window_free_first;
    Win32_Window *window_free_last;
};

struct Mutex {
    CRITICAL_SECTION csection;
};

struct Semaphore {
    HANDLE event;
};




#endif // RTS_OS_WIN32_H
