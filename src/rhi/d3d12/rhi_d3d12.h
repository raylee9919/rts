// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RTS_RHI_D3D12_H
#define RTS_RHI_D3D12_H

#include <windows.h>
#include <dxgi1_6.h>
#include <dxgidebug.h>
#include "third_party/DirectX/Include/d3d12.h"
#include "third_party/DirectX/Include/d3dx12/d3dx12.h"

#include "basic/core.h"
#include "os/os.h"
#include "rhi/rhi_defines.h"

#if USE_PIX
#include "third_party/WinPixRuntimeEvent/Include/pix3.h"
#endif

#define NODE_MASK                       0 // @Todo: Multiple GPUs?
#define RHI_D3D12_SWAP_CHAIN_STEREO     0

struct D3D12_Command_Queue {
    D3D12_COMMAND_LIST_TYPE type;
    ID3D12CommandQueue *queue_0;
};

struct D3D12_Command_List {
    D3D12_COMMAND_LIST_TYPE type;

    ID3D12GraphicsCommandList  *list_0;
    ID3D12GraphicsCommandList7 *list_7;
    
    ID3D12CommandAllocator *allocator;

    b32 is_recording;
};

struct D3D12_Descriptor_Heap {
    D3D12_DESCRIPTOR_HEAP_TYPE  type;

    ID3D12DescriptorHeap       *heap_0;

    RHI_Device                 *device;

    D3D12_CPU_DESCRIPTOR_HANDLE base_cpu_handle;
    D3D12_GPU_DESCRIPTOR_HANDLE base_gpu_handle; // valid if visible on gpu.

    u32                         descriptor_size;
    
    u64                        *free_list;  // set bit means it's free.
    u32                         free_list_node_count;

    u32                         allocated;  // number of every slots.
    u32                         count;      // number of active descriptors.
};

struct D3D12_Descriptor {
    D3D12_DESCRIPTOR_HEAP_TYPE  type;
    u32                         index; // index in the free list.
    D3D12_Descriptor_Heap       *my_heap;
    D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle;
    D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle;
};

struct D3D12_Heap {

};

struct D3D12_Device {
    Allocator               allocator;

    IDXGIFactory6          *dxgi_factory_6;
    ID3D12Device           *device_0;
    ID3D12Device10         *device_10;

    HMODULE                 dxgi_debug_dll_handle;
    IDXGIDebug             *dxgi_debug;

    ID3D12InfoQueue1       *info_queue_1;
    bool                    break_on_warning;
    DWORD                   callback_cookie;

    D3D12_Command_Queue     queues[RHI_COMMAND_TYPE_COUNT];

    D3D12_Descriptor_Heap   rtv_heap;
    D3D12_Descriptor_Heap   dsv_heap;
    D3D12_Descriptor_Heap   resource_heap;
    D3D12_Descriptor_Heap   sampler_heap;

    ID3D12RootSignature    *global_root_signature;
};

struct D3D12_Surface {
    IDXGISwapChain4 *swap_chain_4;
    HANDLE frame_waitable_object;
};

struct D3D12_Buffer {
    ID3D12Resource *resource;
};

struct D3D12_Texture {
    ID3D12Resource *resource;
};

struct D3D12_Fence {
    ID3D12Fence *fence_0;
    HANDLE       event;
};

struct D3D12_Pipeline {
    ID3D12PipelineState *state;
};

bool  d3d12_device_init(RHI_Device *device, bool debug, bool break_on_warning, Allocator allocator);
void  d3d12_device_deinit(RHI_Device *device);

bool  d3d12_command_list_init(RHI_Device *device, RHI_Command_Buffer *cmd_buffer, RHI_Command_Type type);
void  d3d12_command_list_deinit(RHI_Command_Buffer *cmd_buffer);
void  d3d12_command_list_begin(RHI_Command_Buffer *cmd_buffer);
void  d3d12_command_list_end(D3D12_Command_List *list);

void  d3d12_submit(RHI_Device *device, u32 count, RHI_Command_Buffer **cmd_buffers);

void  d3d12_pass_begin(RHI_Command_Buffer *cmd_buffer, RHI_Pass *pass);
void  d3d12_pass_end(RHI_Command_Buffer *cmd_buffer, RHI_Pass *pass);

bool  d3d12_buffer_init(RHI_Device *device, RHI_Buffer *buffer, RHI_Buffer_Desc *desc, RHI_Heap *heap);
void  d3d12_buffer_deinit(RHI_Buffer *buffer);
void *d3d12_buffer_map(RHI_Buffer *buffer);
void  d3d12_buffer_unmap(RHI_Buffer *buffer);

void  d3d12_buffer_view_init(RHI_Device *device, RHI_Buffer_View *view, RHI_Buffer *buffer, RHI_Buffer_View_Desc *desc);
void  d3d12_buffer_view_deinit(RHI_Buffer_View *view);

bool  d3d12_texture_init(RHI_Device *device, RHI_Texture *texture, RHI_Texture_Desc *desc, RHI_Heap *heap);
void  d3d12_texture_deinit(RHI_Texture *texture);

void  d3d12_texture_view_init(RHI_Device *device, RHI_Texture_View *view, RHI_Texture *texture, RHI_Texture_View_Desc *desc);
void  d3d12_texture_view_deinit(RHI_Texture_View *view);

void  d3d12_sampler_init(RHI_Device *device, RHI_Sampler *sampler, RHI_Sampler_Desc *desc);
void  d3d12_sampler_deinit(RHI_Sampler *sampler);

bool  d3d12_surface_init(RHI_Device *device, RHI_Surface *surface, RHI_Surface_Desc *desc, RHI_Texture *out_textures[RHI_MAX_BUFFER_COUNT]);
void  d3d12_surface_present(RHI_Surface *surface, u32 sync_interval);
void  d3d12_surface_resize(RHI_Surface *surface, u32 width, u32 height, RHI_Texture *in_out_textures[RHI_MAX_BUFFER_COUNT]);
bool  d3d12_surface_wait_for_waitable_object(RHI_Surface *surface);

bool  d3d12_fence_init(RHI_Device *device, RHI_Semaphore *fence);
void  d3d12_fence_deinit(RHI_Semaphore *fence);
void  d3d12_fence_wait(RHI_Semaphore *fence, u64 value, s32 milliseconds);
u64   d3d12_fence_completed_value(RHI_Semaphore *fence);

void  d3d12_queue_signal(RHI_Device *device, RHI_Command_Type queue_type, RHI_Semaphore *semaphore, u64 value);
void  d3d12_queue_wait(RHI_Device *device, RHI_Command_Type queue_type, RHI_Semaphore *semaphore, u64 value);

void  d3d12_cmd_texture_barrier(RHI_Command_Buffer *cmd_buffer, RHI_Texture *texture, RHI_Resource_State after, u32 mip_level, u32 array_slice);
void  d3d12_cmd_set_pipeline(RHI_Command_Buffer *cmd_buffer, RHI_Pipeline *pipeline);
void  d3d12_cmd_set_viewport(RHI_Command_Buffer *cmd_buffer, float x, float y, float width, float height, float min_depth, float max_depth);
void  d3d12_cmd_set_scissor(RHI_Command_Buffer *cmd_buffer, u32 x, u32 y, u32 width, u32 height);
void  d3d12_cmd_draw(RHI_Command_Buffer *cmd_buffer, u32 num_vertices, u32 num_instances, u32 first_vertex, u32 first_instance);
void  d3d12_cmd_draw_indexed(RHI_Command_Buffer *cmd_buffer, RHI_Buffer *index_buffer, u32 index_size, u32 num_indices, u32 num_instances, u32 first_index, u32 first_vertex, u32 first_instance);
void  d3d12_cmd_push_constants(RHI_Command_Buffer *cmd_buffer, u32 root_index, void *data, u32 size);
void  d3d12_cmd_copy_buffer_to_buffer(RHI_Command_Buffer *cmd_buffer, RHI_Buffer *dst, RHI_Buffer *src, u64 dst_offset, u64 src_offset, u64 size);
void  d3d12_cmd_copy_buffer_to_texture(RHI_Command_Buffer *cmd_buffer, RHI_Buffer *src, u32 src_offset, u32 src_pitch, RHI_Texture *dst, RHI_Box *box, u32 mip, u32 layer);

bool  d3d12_pipeline_init(RHI_Device *device, RHI_Pipeline *pipeline, RHI_Pipeline_Desc *desc);
void  d3d12_pipeline_deinit(RHI_Pipeline *pipeline);

#endif // RTS_RHI_D3D12_H
