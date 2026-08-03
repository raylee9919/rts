// Copyright Seong Woo Lee. All Rights Reserved.

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

enum RHI_Kind : u8 {
    RHI_KIND_INVALID = 0,
    RHI_KIND_D3D12   = 1,
    RHI_KIND_VULKAN  = 2,
};

struct RHI_Device {
    RHI_Kind kind;
    union {
        D3D12_Device d3d12;
    };
};

struct RHI_Command_Buffer {
    RHI_Kind kind;
    RHI_Command_Type type;
    union {
        D3D12_Command_List d3d12;
    };
};


//
// Heap
//
struct RHI_Heap {
    RHI_Kind kind;
    union {
        D3D12_Heap d3d12;
    };
};


//
// Texture
//
enum RHI_Texture_Type {
    RHI_TEXTURE_TYPE_1D,
    RHI_TEXTURE_TYPE_2D,
    RHI_TEXTURE_TYPE_2D_ARRAY,
    RHI_TEXTURE_TYPE_3D,
    RHI_TEXTURE_TYPE_CUBE,
};

enum RHI_Texture_Format {
    RHI_TEXTURE_FORMAT_UNKNOWN = 0,

    RHI_TEXTURE_FORMAT_R8_UNORM,
    RHI_TEXTURE_FORMAT_RG8_UNORM,
    RHI_TEXTURE_FORMAT_RGBA8_UNORM,
    RHI_TEXTURE_FORMAT_BGRA8_UNORM,
    RHI_TEXTURE_FORMAT_RGBA8_UNORM_SRGB,
    RHI_TEXTURE_FORMAT_BGRA8_UNORM_SRGB,

    RHI_TEXTURE_FORMAT_R16_UNORM,
    RHI_TEXTURE_FORMAT_RG16_UNORM,
    RHI_TEXTURE_FORMAT_RGBA16_UNORM,

    RHI_TEXTURE_FORMAT_R16F,
    RHI_TEXTURE_FORMAT_RG16F,
    RHI_TEXTURE_FORMAT_RGBA16F,

    RHI_TEXTURE_FORMAT_R32F,
    RHI_TEXTURE_FORMAT_RG32F,
    RHI_TEXTURE_FORMAT_RGBA32F,

    RHI_TEXTURE_FORMAT_D32F,

    RHI_TEXTURE_FORMAT_BC1_UNORM,
    RHI_TEXTURE_FORMAT_BC1_UNORM_SRGB,
    RHI_TEXTURE_FORMAT_BC3_UNORM,
    RHI_TEXTURE_FORMAT_BC3_UNORM_SRGB,
    RHI_TEXTURE_FORMAT_BC4_UNORM,
    RHI_TEXTURE_FORMAT_BC5_UNORM,
    RHI_TEXTURE_FORMAT_BC6H_UFLOAT,
    RHI_TEXTURE_FORMAT_BC7_UNORM,
    RHI_TEXTURE_FORMAT_BC7_UNORM_SRGB,
};

typedef u8 RHI_Texture_Usage;
enum {
    RHI_TEXTURE_USAGE_SAMPLED                  = 0x1,
    RHI_TEXTURE_USAGE_STORAGE                  = 0x2,
    RHI_TEXTURE_USAGE_COLOR_ATTACHMENT         = 0x4,
    RHI_TEXTURE_USAGE_DEPTH_STENCIL_ATTACHMENT = 0x8,
};

struct RHI_Texture_Desc {
    RHI_Texture_Type   type;
    RHI_Texture_Format format;
    RHI_Texture_Usage  usage;
    u32 width;
    u32 height;
    u32 mip_levels;
    u32 depth; // or array length.
};

struct RHI_Texture {
    RHI_Kind kind;
    RHI_Texture_Desc desc;
    union {
        D3D12_Texture d3d12;
    };
};

enum RHI_Texture_View_Type {
    RHI_TEXTURE_VIEW_TYPE_SAMPLED,
    RHI_TEXTURE_VIEW_TYPE_UNORDERED_ACCESS,
    RHI_TEXTURE_VIEW_TYPE_RENDER_TARGET,
    RHI_TEXTURE_VIEW_TYPE_DEPTH_STENCIL
};

struct RHI_Texture_View_Desc {
    RHI_Texture_View_Type type;
    RHI_Texture_Type      dimension;
    RHI_Texture_Format    format;
    u32 base_mip_level;
    u32 base_array_slice;
    u32 mip_levels;
    u32 depth; // or array length.
};

struct RHI_Texture_View {
    RHI_Kind kind;
    RHI_Texture_View_Desc desc;
    union {
        D3D12_Descriptor d3d12;
    };
};


//
// Buffer
//

struct RHI_Buffer_View {

};


//
// Surface
//
struct RHI_Surface_Desc {
    void *native_window_handle; // This isn't a pointer to the handle. It's a handle itself.
    u32   width;
    u32   height;
    u32   num_back_buffers;
};

struct RHI_Surface {
    RHI_Kind kind;
    RHI_Surface_Desc desc;
    RHI_Texture textures[RHI_MAX_BACK_BUFFERS];
    union {
        D3D12_Surface d3d12;
    };
};


//
// Render Pass
//
enum RHI_Load_Op {
    RHI_LOAD_OP_LOAD,
    RHI_LOAD_OP_CLEAR,
    RHI_LOAD_OP_DONT_CARE
};

enum RHI_Store_Op {
    RHI_STORE_OP_STORE,
    RHI_STORE_OP_DONT_CARE
};

struct RHI_Attachment {
    RHI_Texture_View view;
    RHI_Load_Op  load_op;
    RHI_Store_Op store_op;
    union {
        f32 clear_color[4];
        f32 clear_depth;
    };
};

struct RHI_Pass {
    RHI_Attachment color_attachments[RHI_MAX_COLOR_ATTACHMENTS];
    RHI_Attachment depth_attachment;
    u32            num_color_attachments;
    b32            has_depth_attachment;
};


//
// Fence
//
struct RHI_Fence { 
    RHI_Kind kind;
    union {
        D3D12_Fence d3d12;
    };
};


//
// API
//
internal bool rhi_device_init(RHI_Device *device, RHI_Kind kind, bool debug, bool break_on_warning);
internal void rhi_device_deinit(RHI_Device *device);

internal bool rhi_command_buffer_init(RHI_Device *device, RHI_Command_Buffer *cmd, RHI_Command_Type type);
internal void rhi_command_buffer_deinit(RHI_Command_Buffer *cmd_buffer);
internal void rhi_command_buffer_begin(RHI_Command_Buffer *cmd_buffer);
internal void rhi_command_buffer_end(RHI_Command_Buffer *cmd_buffer);

internal void rhi_submit(RHI_Device *device, u32 count, RHI_Command_Buffer **cmd_buffers);

internal bool rhi_surface_init(RHI_Device *device, RHI_Surface *surface, RHI_Surface_Desc *desc);
internal void rhi_surface_present(RHI_Surface *surface);

internal bool rhi_texture_create(RHI_Device *device, RHI_Texture *texture, RHI_Texture_Desc *desc, RHI_Heap *heap);
internal void rhi_texture_destroy(RHI_Texture *texture);

internal void rhi_texture_view_create(RHI_Device *device, RHI_Texture_View *view, RHI_Texture *texture, RHI_Texture_View_Desc *desc);
internal void rhi_texture_view_destroy(RHI_Texture_View *view);

internal void rhi_pass_begin(RHI_Command_Buffer *cmd_buffer, RHI_Pass *render_pass);
internal void rhi_pass_end(RHI_Command_Buffer *cmd_buffer, RHI_Pass *render_pass);

internal bool rhi_fence_create(RHI_Device *device, RHI_Fence *fence);
internal void rhi_fence_destroy(RHI_Fence *fence);
internal void rhi_fence_wait(RHI_Fence *fence, u64 value, u64 timeout); // Pass 0xffffffff for inifinite timeout.
internal void rhi_fence_signal(RHI_Fence *fence, u64 value);



#endif // RTS_RHI_H
