// Copyright Seong Woo Lee. All Rights Reserved.

//
// @Note: 1. No ray tracing.
//        2. No mesh shader.
//        3. Bindless makes my life easier.
//

#ifndef RTS_RHI_H
#define RTS_RHI_H


struct RHI_State {
    Arena *arena;
    void  *platform;
};


internal bool rhi_init(RHI_State* rhi, OS_Window window);
internal void r_begin(RHI_State* rhi, v2 window_size, v2 render_size);
internal void r_end(RHI_State* rhi, struct Renderer *r);


// ------------------------------------------------------------------------ //


typedef u8 RHI_Kind;
enum {
    RHI_KIND_INVALID = 0,
    RHI_KIND_OPENGL  = 1,
    RHI_KIND_VULKAN  = 2,
    RHI_KIND_D3D11   = 3,
    RHI_KIND_D3D12   = 4,
    RHI_KIND_METAL   = 5,
};

struct RHI_Device {
    RHI_Kind kind;

    union {
        D3D12_Device d3d12;
    };
};


internal bool rhi_device_init(RHI_Device *device, RHI_Kind kind, bool debug, bool break_on_warning);
internal void rhi_device_deinit(RHI_Device *device);


#endif // RTS_RHI_H
