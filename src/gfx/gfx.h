// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RTS_GFX_H
#define RTS_GFX_H

#define GFX_PASS_NULL               0xffffffffffffffff
#define GFX_PIPELINE_INVALID        UINT64_MAX

#define GFX_FIRST_PUSH_CONSTANTS    0

// Minimum and maximum count of set of frame resources.
#define GFX_MIN_FRAME_COUNT         1
#define GFX_MAX_FRAME_COUNT         2


// Sort keys
//
typedef u16 GFX_Key;
enum {
    GFX_KEY_PASS = 0,
    GFX_KEY_PIPELINE,
    GFX_KEY_CONSTANTS,

    GFX_KEY_COUNT
};

global read_only constexpr u64 gfx_key_lengths[GFX_KEY_COUNT] = {
    5, 16, 16
};
global read_only constexpr u64 gfx_key_offsets[GFX_KEY_COUNT] = {
    0,
    gfx_key_lengths[0],
    gfx_key_lengths[0] + gfx_key_lengths[1],
};

global read_only constexpr u64 GFX_MAX_PASS      = (1ull << gfx_key_lengths[GFX_KEY_PASS]);
global read_only constexpr u64 GFX_MAX_PIPELINES = (1ull << gfx_key_lengths[GFX_KEY_PIPELINE]);

static_assert([] {
    u64 sum = 0;
    for (u64 v : gfx_key_lengths)  sum += v;
    return sum <= 64;
}(), "gfx: sum of key lengths must be <= 64.");

struct GFX_Sort_Key {
    u64 bits;      // Packed keys
    u32 cmd_index; // Index to the actual command
};


// GUID to uint32
//
static force_inline u32 gfx_128_to_32(Guid guid) {
    // @Todo: Not critical as it's just used for hash table index, but is there a better way?
    return guid._32[0] ^ guid._32[1] ^ guid._32[2] ^ guid._32[3];
}


// Initialization info struct
//
struct GFX_Info {
    RHI_Kind    kind;
    b32         debug;
    b32         break_on_warning;

    void        *native_window_handle;
    u32         width;
    u32         height;

    u32         num_buffers;
    u32         num_frames;

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
    RHI_Texture      texture;
    RHI_Texture_View views[RHI_TEXTURE_VIEW_TYPE_COUNT];
};

struct GFX_Pipeline_Entry {
    RHI_Pipeline    rhi_pipeline;
};

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

struct GFX_Material {
    v3          albedo         = v3{1.f, 1.f, 1.f};
    f32         metallic       = 0.f;
    f32         roughness      = 1.f;

    Guid        albedo_texture = NULL_GUID;
    Guid        orm_texture    = NULL_GUID;
};

struct GFX_Pass_State {
    GFX_Pass_Flags      flags;

    GFX_Viewport        viewport;
    GFX_Scissor         scissor;

    Guid                color_attachments[RHI_MAX_COLOR_ATTACHMENTS];
    Guid                depth_attachment;

    u32                 colors[RHI_MAX_COLOR_ATTACHMENTS];
    f32                 depth;
};

struct GFX_Command {
    u64          pipeline_index;
    u32          push_constant_index;
    Guid         mesh_handle;
    u32          num_instances;
};

struct GFX_Callback_Entry {
    u64 semaphore_value_to_execute;
    void (*proc)(GFX_Callback_Entry entry);
    union {
        Guid mesh_id;
    };
};

struct GFX_Push_Constants {
    u32 data[RHI_MAX_32BIT_PUSH_CONSTANTS];
    u32 size;
};

struct GFX_State {
    Allocator                               arena;

    bool                                    initted = false;
    bool                                    should_shutdown = false;

    RHI_Device                              *device;
    GFX_Info                                info;
    u32                                     generational_handle_id   = 1; // so null handle is never generated.
    u64                                     generational_pipeline_id = 1; // pipeline bits in the sort key.

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

    RHI_Semaphore                           frame_semaphore; // frame semaphore.
    u64                                     current_frame = 1;

    // I'll just have a single swapchain.
    RHI_Surface                             *surface;
    RHI_Texture_View                        surface_views[RHI_MAX_BUFFER_COUNT];

    RHI_Sampler                             linear_sampler;

    // @Temporary: Wait on the spot is the worst possible way.
    RHI_Buffer                              upload_buffer;
    RHI_Semaphore                           upload_semaphore;
    u64                                     upload_semaphore_value = 1;
    RHI_Command_Buffer                      copy_buffer; // One copy buffer should be enough. Right?

    RHI_Command_Buffer                      command_buffers[GFX_MAX_FRAME_COUNT];
    RHI_Command_Buffer                      compute_buffers[GFX_MAX_FRAME_COUNT];

    // gfx's draw calls encode commands into the buffer by the current context.
    // Later commands get sorted by key and submitted to the GPU.
    u64                                     current_pass = GFX_PASS_NULL;

    // Transient pipeline frame data
    u64                                     context_pipeline = GFX_PIPELINE_INVALID;  // Currently set transient pipeline index
    u64                                     next_pipeline    = 0;                     // Next transient pipeline index to acquire
    Array<Guid>                             pipelines;                                // pipelines[transient pipeline index] = guid
    Table <Guid, u64, gfx_128_to_32>        pipeline_to_index_this_frame;             // increments index if new pipeline was encountered this frame

    u64                                     current_push_constants = 0;
    Array<GFX_Push_Constants>               push_constants;

    Array<GFX_Sort_Key>                     sort_keys;
    Array<GFX_Command>                      commands;
    GFX_Pass_State                          pass_states[GFX_MAX_PASS];

    // Resource tables
    Table <Guid,           GFX_Mesh, gfx_128_to_32>     mesh_table;
    Table <Guid,       GFX_Material, gfx_128_to_32>     material_table;
    Table <Guid,  GFX_Texture_Entry, gfx_128_to_32>     texture_table;
    Table <Guid, GFX_Pipeline_Entry, gfx_128_to_32>     pipeline_table;

    // Shader global data
    f32 time;

    // Resize
    b32 resize_requested = false;
    u32 resize_width;
    u32 resize_height;

    // Framebuffer Depth Textures
    Guid depth_textures[RHI_MAX_BUFFER_COUNT];
};

global GFX_State *gfx;
global Guid GFX_SURFACE_TEXTURE;



internal void                   gfx_init(GFX_Info info);
internal void                   gfx_shutdown();

internal void                   gfx_mesh_create(Guid guid, void *vertices, u32 num_vertices, u32 vertex_size, void *indices, u32 num_indices, u32 index_size);
internal void                   gfx_mesh_destroy(Guid guid);

internal void                   gfx_material_alloc(Guid guid, GFX_Material material);
internal void                   gfx_material_dealloc(Guid guid);
internal GFX_Material           *gfx_get_material_pointer_from_guid(Guid guid);

internal void                   gfx_texture_create(Guid guid, RHI_Texture_Desc desc);
internal void                   gfx_texture_destroy(Guid guid);
internal void                   gfx_texture_upload(Guid guid, RHI_Format format, void *data, u32 size, u32 width, u32 height);
internal u32                    gfx_bindless_from_texture(Guid guid, RHI_Texture_View_Type view_type);

// The last state you set will be submitted to the GPU. The system isn't smart 
// enough to untangle the order in which you called them.
internal void                   gfx_pass_begin(u32 pass_index);
internal void                   gfx_pass_end();
internal void                   gfx_pass_color_attachment(u32 pass_index, u32 color_attachment_index, Guid texture);
internal void                   gfx_pass_depth_attachment(u32 pass_index, Guid texture);
internal void                   gfx_pass_clear_color(u32 pass_index, u32 clear_color, u32 color_attachment_index);
internal void                   gfx_pass_clear_depth(u32 pass_index, f32 clear_depth);

// Sets viewport and scissor of currently set pass.
internal void                   gfx_set_viewport(f32 top_left_x, f32 top_left_y, f32 width, f32 height);
internal void                   gfx_set_scissor(u32 top_left_x, u32 top_left_y, u32 width, u32 height);

internal void                   gfx_pipeline_create(Guid guid, RHI_Pipeline_Desc desc);
internal void                   gfx_pipeline_destroy(Guid guid);
internal void                   gfx_set_pipeline(Guid guid);

internal void                   gfx_push_constants(void *data, u32 size);

internal void                   gfx_draw(Guid mesh_id);

internal void                   gfx_end(f64 dt, u32 sync_interval);

internal bool                   gfx_wait_for_frame_waitable_object();

internal void                   gfx_request_swapchain_resize(u32 width, u32 height);

internal GPU_Material           gpu_material_from_gfx(GFX_Material *material);


#endif // RTS_GFX_H
