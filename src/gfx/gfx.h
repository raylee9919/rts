// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RTS_GFX_H
#define RTS_GFX_H

struct GFX_Init {
    RHI_Kind kind;
    b32 debug;
    b32 break_on_warning;

    void *native_window_handle;
    u32 width;
    u32 height;
    u32 num_back_buffers;
};

struct GFX_Mesh { 
    RHI_Buffer      vertex_buffer;
    RHI_Buffer_View vertex_buffer_view;
    RHI_Buffer      index_buffer;
};

struct GFX_State {
    Arena       *arena;
    RHI_Device  *device;
    GFX_Init    init;

    // I'll just have a single swapchain.
    RHI_Surface *surface;

    RHI_Sampler linear_sampler;

    Table<u64, GFX_Mesh> mesh_table;

    // @Temporary: Wait on the spot is the worst synchronization.
    RHI_Buffer          upload_buffer;
    RHI_Semaphore       upload_semaphore;
    u64                 upload_semaphore_value = 1;
    RHI_Command_Buffer  copy_buffer; // One copy buffer should be enough. Right?

    RHI_Command_Buffer  command_buffers[RHI_MAX_BACK_BUFFERS];
    RHI_Command_Buffer  compute_buffers[RHI_MAX_BACK_BUFFERS];
};
global GFX_State *gfx;


internal void gfx_init(GFX_Init init);
internal void gfx_shutdown();

internal void gfx_register_mesh(u64 id, void *vertices, u32 num_vertices, u32 vertex_size, void *indices, u32 num_indices, u32 index_size);
internal void gfx_unregister_mesh(u64 id);

internal void gfx_draw(u64 mesh_id);

internal void gfx_end();


#endif // RTS_GFX_H
