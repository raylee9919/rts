// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RTS_OS_WIN32_H
#define RTS_OS_WIN32_H

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

//
// DXGI
//
#pragma comment(lib, "dxgi")
#pragma comment(lib, "dxguid")
#include <dxgi1_6.h>
#include <dxgidebug.h>

//
// D3D12
//
#pragma comment(lib, "d3d12")
#include <ThirdParty/DirectX/Include/d3d12.h>
#include <ThirdParty/DirectX/Include/d3dx12/d3dx12.h>




typedef CRITICAL_SECTION Critical_Section;



#endif // RTS_OS_WIN32_H
