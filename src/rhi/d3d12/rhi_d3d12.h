// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RTS_RHI_D3D12_H
#define RTS_RHI_D3D12_H

struct RHI_Device;

struct D3D12_Device {
    ID3D12Device10 *device;
    IDXGIFactory6  *factory;
};


internal bool d3d12_device_init(RHI_Device *device, bool debug);
internal void d3d12_device_deinit(RHI_Device *device);

#endif // RTS_RHI_D3D12_H
