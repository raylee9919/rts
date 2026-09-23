// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RTS_GFX_H
#define RTS_GFX_H

#include "basic/core.h"
#include "basic/allocator.h"
#include "basic/array.h"
#include "basic/queue.h"
#include "basic/hash_table.h"
#include "math/math.h"
#include "os/os.h"
#include "rhi/rhi.h"
#include "shaders/shared/shared.h"


#define GFX_INVALID                 UINT64_MAX
#define GFX_MAX_PASS                64
#define GFX_SCENE_DEPTH_FORAMT      RHI_FORMAT_D32F


/* Sort Keys */
typedef u16 GFX_Key;
enum {
    // The least significant key is at the top.
    GFX_KEY_CONSTANTS,
    GFX_KEY_PIPELINE,

    GFX_KEY_COUNT
};

// Each bitfield's length in sort key.
global read_only constexpr u64 gfx_key_lengths[GFX_KEY_COUNT] = {
    16, 16
};

// Each bitfield's offset in sort key. Filled at initialization.
extern u64 gfx_key_offsets[GFX_KEY_COUNT];

global read_only constexpr u64 GFX_MAX_PIPELINES        = (1ull << gfx_key_lengths[GFX_KEY_PIPELINE]);
global read_only constexpr u64 GFX_MAX_PUSH_CONSTANTS   = (1ull << gfx_key_lengths[GFX_KEY_CONSTANTS]);

static_assert([] {
    u64 sum = 0;
    for (u64 v : gfx_key_lengths)  sum += v;
    return sum <= 64;
}(), "gfx: sum of key lengths must be <= 64.");

struct GFX_Sort_Key {
    u64 bits;      // Packed keys
    u32 cmd_index; // Index to the actual command
};


// Initialization info struct
//
struct GFX_Info {
    RHI_Kind    kind;
    b32         debug;
    b32         break_on_warning;

    void       *native_window_handle;
    u32         width;
    u32         height;

    b32         vsync_off;

    b32         frame_latency_waitable;
};

struct GFX_Mesh { 
    RHI_Buffer      vertex_buffer;
    RHI_Buffer_View vertex_buffer_view;
    RHI_Buffer      index_buffer;
    u32             index_size;
    u32             num_indices;
};

struct GFX_Texture_Entry {
    RHI_Texture         texture;
    RHI_Texture_View    srv;
    RHI_Texture_View    uav;
};

struct GFX_Pipeline_Entry {
    RHI_Pipeline    rhi_pipeline;
};

struct GFX_Viewport {
    f32 x, y, w, h; // top-left x and y.
};

struct GFX_Scissor {
    u32 x, y, w, h; // top-left x and y.
};

struct GFX_Pass {
    String          name;
    GFX_Viewport    viewport;
    GFX_Scissor     scissor;
    Guid            color_attachments[RHI_MAX_COLOR_ATTACHMENTS];
    Guid            depth_attachment;
    f32             min_depth;
    f32             max_depth;
};

struct GFX_Command {
    Guid         mesh_handle;
    u32          num_instances;
};

struct GFX_Callback_Entry {
    u64 semaphore_value_to_execute;
    void (*proc)(GFX_Callback_Entry entry);
    union {
        Guid mesh_id;
        RHI_Texture_View view;
    };
};

struct GFX_Push_Constants {
    u32 data[RHI_MAX_32BIT_PUSH_CONSTANTS];
    u32 size;
};

struct GFX_Edge {
    Guid                resource;
    u32                 dst_pass;
    RHI_Resource_State  dst_state;
};


struct GFX_State {
    Allocator                               arena;
    Allocator                               heap;

    RHI_Device                              *device;
    GFX_Info                                info;

    // The thread will periodically check the 'frame' semaphore at the end of the 
    // frames and run callbacks in the queue whose semaphore value is less or equal 
    // to the semaphore value that is checked to be completed. Releasing resource 
    // early is a problem, but releasing 1,2 frames late? I won't say that's a concern.
    // If it turns out to be a bad idea, I'll make a dedicated thread or something.
    //
    // @Todo:
    // Case 1.
    //      
    //      API Timeline:
    //          x = CreateMesh(A) -> Draw(x) -> DestroyMesh(x) -> y = CreateMesh(A) -> Draw(y)
    //
    //      Actual Timeline:
    //          [ x = CreateMesh(A) -> y = CreateMesh(A) ] -> Draw(x) -> Draw(y)
    //                           Immediately.              -> [ Draw(x) -> Draw(y) ]
    //                                                              After sort         -> DestroyMesh(x)
    //                                                                                      Callback
    //      
    //      
    //
    //
    Queue<GFX_Callback_Entry>               callbacks;

    RHI_Semaphore                           frame_semaphore;
    u64                                     current_frame = 1; // Indefinitely increases

    // I'll just have a single swapchain.
    RHI_Surface                            *surface;
    Guid                                    surface_guids[RHI_MAX_BUFFER_COUNT];

    RHI_Sampler                             linear_sampler;
    RHI_Sampler                             dot_sampler;

    // @Fix
    // At the moment, queues are waiting for the upload semaphore 
    // only once at gfx_end. Proper API and alignment is required.
    // Seems like command buffer pool is required? Also, this isn't 
    // thread safe.
    //         -swl 2026-09-23
    RHI_Buffer                              upload_buffer;
    RHI_Semaphore                           upload_semaphore;
    u64                                     upload_semaphore_value = 1;
    u8                                     *upload_buffer_mapped;
    u64                                     upload_buffer_used;

    RHI_Command_Buffer                      copy_buffer; // One copy buffer should be enough. Right? Nope.
    RHI_Command_Buffer                      command_buffers[RHI_MAX_BUFFER_COUNT];
    RHI_Command_Buffer                      compute_buffers[RHI_MAX_BUFFER_COUNT];

    // gfx's draw calls encode commands into the buffer by the current context.
    // Later commands get sorted by key and submitted to the GPU.
    u64                                     context_pass = GFX_INVALID;


    // Transient pipeline frame data
    u64                                     context_pipeline = GFX_INVALID; // Currently set transient pipeline index. Draw calls incorporates this.
    u64                                     next_pipeline    = 0;           // Next transient pipeline index to acquire.
    Array<Guid>                             pipelines;                      // pipelines[transient pipeline index] = guid.
    Table <Guid, u64, hash_guid>            pipeline_to_index_this_frame;   // increments index if new pipeline was encountered this frame.


    // Transient push constants data
    u64                                     context_push_constants = GFX_INVALID;
    u64                                     next_push_constants    = 0;
    Array<GFX_Push_Constants>               push_constants;
    Table <u64, u64>                        push_constants_to_index_this_frame;


    // Buckets of sort keys and commands. Index GFX_MAX_PASS is reserved for NIL.
    Array<GFX_Sort_Key>                     sort_keys[GFX_MAX_PASS + 1];
    Array<GFX_Command>                      commands[GFX_MAX_PASS + 1];

    // Pass
    GFX_Pass pass_states[GFX_MAX_PASS + 1];

    // Resource tables
    Table <Guid,           GFX_Mesh, hash_guid>     mesh_table;
    Table <Guid,  GFX_Texture_Entry, hash_guid>     texture_table;
    Table <Guid, GFX_Pipeline_Entry, hash_guid>     pipeline_table;

    // Shader global data
    f32 time;

    // Resize
    b32 resize_requested = false;
    u32 resize_width;
    u32 resize_height;

    // Frame Graph
    Array<GFX_Edge> out_edges[GFX_MAX_PASS + 1]; // Index 'GFX_MAX_PASS' is nil
};

extern GFX_State *gfx;



void gfx_init(GFX_Info info, u32 num_backbuffers);
void gfx_shutdown();

// Reserves 'size' bytes of the upload buffer and returns the offset. If it doesn't
// fit, blocks until previous uploads are complete and wraps around to the start.
u64  gfx_upload_reserve(u64 size, u64 alignment);

void gfx_mesh_create(Guid guid, void *vertices, u32 num_vertices, u32 vertex_size, void *indices, u32 num_indices, u32 index_size);
void gfx_mesh_destroy(Guid guid);

// Creates texture, SRV and UAV according to the desc's usage flags.
void gfx_texture_create(Guid guid, RHI_Texture_Desc desc);

// Destroy texture. Ignores the request if the given ID is not found.
void gfx_texture_destroy(Guid guid);

void gfx_texture_upload(Guid guid, RHI_Format format, void *data, u32 size, u32 width, u32 height);

// Get SRV from texture GUID. Returns nullptr if texture doesn't exist or SRV wasn't created.
RHI_Texture_View *gfx_srv_from_texture(Guid guid);

// Get UAV from texture GUID. Returns nullptr if texture doesn't exist or UAV wasn't created.
RHI_Texture_View *gfx_uav_from_texture(Guid guid);

// Get bindless handle of SRV from texture GUID. returns GFX_INVALID_BINDLESS if it doesn't exist.
u32 gfx_srv_bindless_from_texture(Guid guid);

// Get bindless handle of UAV from texture GUID. returns GFX_INVALID_BINDLESS if it doesn't exist.
u32 gfx_uav_bindless_from_texture(Guid guid);

// The last pass state you set will be submitted to the GPU. The system isn't
// smart enough to untangle the order in which you called them.
void gfx_pass_begin(u32 pass_index, GFX_Pass *pass);
void gfx_pass_end();

// Immediate-mode pass connection built every frame. No need to disconnect manually.
// Pass 'src_pass' as -1 to indicate the pass with no dependencies.
void gfx_pass_connect(Guid resource, u32 src_pass, u32 dst_pass, RHI_Resource_State dst_state);

// Returns the swapchain's backbuffer index for this frame.
u32 gfx_backbuffer_index();

// Returns the swapchain's texture for this frame.
Guid gfx_surface_texture();

// Returns current number of backbuffers.
u32 gfx_backbuffer_count();

// Returns Swapchain textures' format.
RHI_Format gfx_surface_format();


void gfx_pipeline_create(Guid guid, RHI_Pipeline_Desc desc);
void gfx_pipeline_destroy(Guid guid);
void gfx_set_pipeline(Guid guid);

void gfx_push_constants(void *data, u32 size);

void gfx_draw(Guid mesh_id, u32 num_instances);


void gfx_begin();
void gfx_end(f64 dt, u32 sync_interval);

bool gfx_wait_for_frame_waitable_object();

void gfx_request_swapchain_resize(u32 width, u32 height);


#endif // RTS_GFX_H
