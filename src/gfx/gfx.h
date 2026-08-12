// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RTS_GFX_H
#define RTS_GFX_H

#define GFX_MAX_PASS 32 
static_assert((((GFX_MAX_PASS - 1) & GFX_MAX_PASS) == 0) && (GFX_MAX_PASS > 0), 
              "GFX_MAX_PASS must be power of two.");


// @Robustness
read_only global const u64 gfx_sort_key_bitmask_pass   = ((1 <<  5) - 1);                             // [ 0:  4]
read_only global const u64 gfx_sort_key_bitmask_shader = ((1 << 13) - 1) ^ gfx_sort_key_bitmask_pass; // [ 5: 12]
static_assert((0
               | gfx_sort_key_bitmask_pass
               | gfx_sort_key_bitmask_shader
              ) == (0
                    ^ gfx_sort_key_bitmask_pass
                    ^ gfx_sort_key_bitmask_shader
                   ));

struct GFX_Sort_Key {
    u64 pass;
    u64 shader;
};

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

struct GFX_Texture {
    RHI_Texture      texture;
    RHI_Texture_View views[RHI_TEXTURE_VIEW_TYPE_COUNT];
};

struct GFX_Texture_Handle {
    u64 u[1];

    bool operator == (GFX_Texture_Handle other) {
        return u[0] == other.u[0];
    }

    bool operator != (GFX_Texture_Handle other) {
        return u[0] != other.u[0];
    }
};

static u32 gfx_texture_handle_hash(GFX_Texture_Handle handle) {
    u32 seed = 0xCafeF00d;
    return (u32)(knuth_hash(handle.u[0] ^ seed) >> 32);
}

struct GFX_Command {

};

// Pass state
//
struct GFX_Viewport {
    b32 set;
    f32 x, y, w, h; // top-left x and y.
};

struct GFX_Scissor {
    b32 set;
    u32 x, y, w, h; // top-left x and y.
};

typedef u16 GFX_Pass_Flags;
enum {
    // CLEAR_COLOR flags 'MUST' be in sequential order.
    GFX_PASS_FLAG_CLEAR_COLOR_0       = (1 << 0),
    GFX_PASS_FLAG_CLEAR_COLOR_1       = (1 << 1),
    GFX_PASS_FLAG_CLEAR_COLOR_2       = (1 << 2),
    GFX_PASS_FLAG_CLEAR_COLOR_3       = (1 << 3),
    GFX_PASS_FLAG_CLEAR_COLOR_4       = (1 << 4),
    GFX_PASS_FLAG_CLEAR_COLOR_5       = (1 << 5),
    GFX_PASS_FLAG_CLEAR_COLOR_6       = (1 << 6),
    GFX_PASS_FLAG_CLEAR_COLOR_7       = (1 << 7),
    GFX_PASS_FLAG_CLEAR_COLOR_MAX_OPL = (1 << 8),

    GFX_PASS_FLAG_CLEAR_DEPTH_STENCIL = (1 << 9),
};
static_assert(GFX_PASS_FLAG_CLEAR_COLOR_MAX_OPL == (1 << RHI_MAX_COLOR_ATTACHMENTS));

struct GFX_Pass_State {
    GFX_Pass_Flags      flags;

    GFX_Viewport        viewport;
    GFX_Scissor         scissor;

    GFX_Texture_Handle  color_attachments[RHI_MAX_COLOR_ATTACHMENTS];
    GFX_Texture_Handle  depth_attachment;

    u32                 colors[RHI_MAX_COLOR_ATTACHMENTS];
    f32                 depth;
};


// Monolithic state
//
struct GFX_State {
    Arena                                   *arena;
    RHI_Device                              *device;
    GFX_Init                                init;
    u32                                     generational_id = 1;

    // I'll just have a single swapchain.
    RHI_Surface                             *surface;
    RHI_Texture_View                        surface_views[RHI_MAX_BACK_BUFFERS];
    RHI_Semaphore                           semaphore; // frame semaphore.
    u64                                     current_frame_index;

    RHI_Sampler                             linear_sampler;

    // @Temporary: Wait on the spot is the worst possible way.
    RHI_Buffer                              upload_buffer;
    RHI_Semaphore                           upload_semaphore;
    u64                                     upload_semaphore_value = 1;
    RHI_Command_Buffer                      copy_buffer; // One copy buffer should be enough. Right?

    RHI_Command_Buffer                      command_buffers[RHI_MAX_BACK_BUFFERS];
    RHI_Command_Buffer                      compute_buffers[RHI_MAX_BACK_BUFFERS];


    // gfx's draw calls encode commands into the buffer by the current context.
    // Later commands get sorted by key and submitted to the GPU.
    s64                                     current_pass = -1; // -1 if invalid.
    Array<GFX_Command>                      commands;
    GFX_Pass_State                          pass_states[GFX_MAX_PASS];


    Table<u64, GFX_Mesh>                                             mesh_table;
    Table<GFX_Texture_Handle, GFX_Texture, gfx_texture_handle_hash>  texture_table;
};

global GFX_State *gfx;
global GFX_Texture_Handle GFX_SURFACE_HANDLE;



internal void                   gfx_init(GFX_Init init);
internal void                   gfx_shutdown();

internal void                   gfx_mesh_create(u64 id, void *vertices, u32 num_vertices, u32 vertex_size, void *indices, u32 num_indices, u32 index_size);
internal void                   gfx_mesh_destroy(u64 id);

internal GFX_Texture_Handle     gfx_texture_create(RHI_Texture_Desc desc);
internal void                   gfx_texture_destroy(GFX_Texture_Handle handle);
internal void                   gfx_texture_upload(GFX_Texture_Handle handle, RHI_Texture_Format format, void *data, u32 size, u32 width, u32 height);


// The last state you set will be submitted to the GPU. The system isn't smart 
// enough to untangle the order in which you called them.
internal void                   gfx_pass_color_attachment(u32 pass_index, u32 color_attachment_index, GFX_Texture_Handle texture);
internal void                   gfx_pass_depth_attachment(u32 pass_index, GFX_Texture_Handle texture);
internal void                   gfx_pass_viewport(u32 pass_index, f32 top_left_x, f32 top_left_y, f32 width, f32 height);
internal void                   gfx_pass_scissor(u32 pass_index, u32 top_left_x, u32 top_left_y, u32 width, u32 height);
internal void                   gfx_pass_clear_color(u32 pass_index, u32 clear_color, u32 color_attachment_index);
internal void                   gfx_pass_clear_depth(u32 pass_index, f32 clear_depth);

internal void                   gfx_draw(u64 mesh_id);

internal void                   gfx_end();


#endif // RTS_GFX_H
