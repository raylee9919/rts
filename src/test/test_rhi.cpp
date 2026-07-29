// Copyright Seong Woo Lee. All Rights Reserved.

#include "Basic/include.h"
#include "Math/include.h"
#include "OS/include.h"
#include "RHI/include.h"

#include "Basic/include.cpp"
#include "Math/include.cpp"
#include "OS/include.cpp"
#include "RHI/include.cpp"

int main_entry(int argc, char **argv)
{
    RHI_Device *rhi_device = (RHI_Device *)alloc(sizeof(RHI_Device));
    Assert(rhi_device_init(rhi_device, RHI_KIND_D3D12, true, true));

    rhi_device_deinit(rhi_device);

    return 0;
}
