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

typedef struct D3D12_Device D3D12_Device;

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

struct RHI_Command_List {
    RHI_Kind kind;

    union {
        D3D12_Command_List d3d12;
    };
};

struct RHI_Surface {
    RHI_Kind kind;
    RHI_Surface_Desc desc;

    union {
        D3D12_Surface d3d12;
    };
};


internal bool rhi_device_init(RHI_Device *device, RHI_Kind kind, bool debug, bool break_on_warning);
internal void rhi_device_deinit(RHI_Device *device);

internal bool rhi_command_list_init(RHI_Device *device, RHI_Command_List *list, RHI_Command_Type type);
internal void rhi_command_list_deinit(RHI_Command_List *list);

internal bool rhi_surface_init(RHI_Device *device, RHI_Surface *surface, RHI_Surface_Desc desc);
internal void rhi_surface_present(RHI_Surface *surface);


#endif // RTS_RHI_H
