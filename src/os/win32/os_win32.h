// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RTS_OS_WIN32_H
#define RTS_OS_WIN32_H

#define COM_SAFE_RELEASE(ppT) if (*(ppT)) { (*(ppT))->Release(); *(ppT) = NULL; }

#pragma comment(lib, "user32")
#pragma comment(lib, "gdi32")
#pragma comment(lib, "uxtheme")
#pragma comment(lib, "shell32")
#pragma comment(lib, "winmm")
#pragma comment(lib, "ole32")
#pragma comment(lib, "rpcrt4")

extern "C" 
{
    __declspec(dllexport) DWORD NvOptimusEnablement = 1;
    __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}

// DXGI, D3D12, DXC
#pragma comment(lib, "dxgi")
#pragma comment(lib, "dxguid")
#pragma comment(lib, "d3d12")
#pragma comment(lib, "dxcompiler")

// PIX
#if BUILD_PROFILE
# define USE_PIX 1
# pragma comment(lib, "WinPixEventRuntime")
#endif

typedef CRITICAL_SECTION Critical_Section;


struct Win32_Window {
    Win32_Window    *next;
    Win32_Window    *prev;
    HWND            handle;
    WINDOWPLACEMENT placement;

    b32             keyboard_focused;
};

struct Win32_State {
    Arena *window_arena;
    Win32_Window *window_first;
    Win32_Window *window_last;
    Win32_Window *window_free_first;
    Win32_Window *window_free_last;

    uint64_t    qpc_frequency;
};

struct Mutex {
    SRWLOCK lock;
};

struct Condvar {
    CONDITION_VARIABLE var;
};

struct Semaphore {
    HANDLE event;
};




#endif // RTS_OS_WIN32_H
