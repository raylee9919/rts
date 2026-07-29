// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RTS_RHI_D3D12_H
#define RTS_RHI_D3D12_H

extern "C"
{
    __declspec(dllexport) extern const u32 D3D12SDKVersion = 619;
    __declspec(dllexport) extern const char *D3D12SDKPath = ".\\.";
}

struct RHI_Device;

struct D3D12_Device {
    IDXGIFactory6  *dxgi_factory_6;
    ID3D12Device   *device_0;
    ID3D12Device10 *device_10;

    HMODULE        dxgi_debug_dll_handle;
    IDXGIDebug     *dxgi_debug;


    ID3D12InfoQueue1 *info_queue_1;
    bool  break_on_warning;
    DWORD callback_cookie;
};


internal bool d3d12_device_init(RHI_Device *device, bool debug, bool break_on_warning);
internal void d3d12_device_deinit(RHI_Device *device);

#endif // RTS_RHI_D3D12_H
