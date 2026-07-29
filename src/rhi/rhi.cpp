// Copyright Seong Woo Lee. All Rights Reserved.


bool rhi_device_init(RHI_Device *device, RHI_Kind kind, bool debug, bool break_on_warning) {
    memset(device, 0, sizeof(*device));
    device->kind = kind;

    switch (kind) {
        case RHI_KIND_D3D12:
        return d3d12_device_init(device, debug, break_on_warning);

        default:
        Assert(0);
    }

    return false;
}

void rhi_device_deinit(RHI_Device *device) {
    switch (device->kind) {
        case RHI_KIND_D3D12:
        d3d12_device_deinit(device);
        break;

        default:
        Assert(0);
    }
}
