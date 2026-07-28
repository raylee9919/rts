// Copyright Seong Woo Lee. All Rights Reserved.

extern "C"
{
    __declspec(dllexport) extern const u32 D3D12SDKVersion = 619;
    __declspec(dllexport) extern const char *D3D12SDKPath = ".\\.";
}

bool d3d12_device_init(RHI_Device *device, bool debug) {
    Assert(device->kind == RHI_KIND_D3D12);

    return true;
}

void d3d12_device_deinit(RHI_Device *device) {
    Assert(device->kind == RHI_KIND_D3D12);

    device->d3d12.device->Release();
    device->d3d12.device = 0;

    device->d3d12.factory->Release();
    device->d3d12.factory = 0;
}
