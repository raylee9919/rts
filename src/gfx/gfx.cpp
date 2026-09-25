// Copyright Seong Woo Lee. All Rights Reserved.

#include "gfx/gfx.h"
#include "basic/arena.h"
#include "basic/log.h"
#include "os/os.h"
#include "profiler/profiler.h"

#include "third_party/xxhash3/xxhash.h"

GFX_State *gfx;
u64        gfx_key_offsets[GFX_KEY_COUNT] = { 0 };

static RHI_Texture *rhi_texture_from_guid(Guid guid) {
    GFX_Texture_Entry *entry = table_find_pointer(&gfx->texture_table, guid);
    if (entry)  return &entry->texture;

    return nullptr;
}

static void gfx_init_samplers() {
    {
        RHI_Sampler_Desc desc = {};
        desc.filter             = RHI_FILTER_LINEAR;
        desc.address_u          = RHI_ADDRESS_REPEAT;
        desc.address_v          = RHI_ADDRESS_REPEAT;
        desc.address_w          = RHI_ADDRESS_REPEAT;
        desc.compare_op         = RHI_COMPARE_ALWAYS;
        desc.mip_lod_bias       = 0.f;
        desc.min_lod            = 0.f;
        desc.max_lod            = 1e10f;
        rhi_sampler_init(gfx->device, &gfx->linear_sampler, &desc);
    }
    {
        RHI_Sampler_Desc desc = {};
        desc.filter             = RHI_FILTER_NEAREST;
        desc.address_u          = RHI_ADDRESS_REPEAT;
        desc.address_v          = RHI_ADDRESS_REPEAT;
        desc.address_w          = RHI_ADDRESS_REPEAT;
        desc.compare_op         = RHI_COMPARE_ALWAYS;
        desc.mip_lod_bias       = 0.f;
        desc.min_lod            = 0.f;
        desc.max_lod            = 1e10f;
        rhi_sampler_init(gfx->device, &gfx->dot_sampler, &desc);
    }
}

static void gfx_deinit_samplers() {
    rhi_sampler_deinit(&gfx->dot_sampler);
    rhi_sampler_deinit(&gfx->linear_sampler);
}

static void gfx_swapchain_init(GFX_Info info, 
                               u32 num_back_buffers, 
                               RHI_Texture *out_textures[RHI_MAX_BUFFER_COUNT]) {
    RHI_Surface_Desc desc = {};
    desc.native_window_handle   = info.native_window_handle;
    desc.width                  = info.width;
    desc.height                 = info.height;
    desc.num_back_buffers       = num_back_buffers;
    desc.frame_latency_waitable = info.frame_latency_waitable;

    R_ASSERT(rhi_surface_init(gfx->device, gfx->surface, &desc, out_textures));
}

static void gfx_swapchain_deinit() {
    // rhi_surface_deinit(gfx->surface);
}

static void gfx_swapchain_resize(u32 width, u32 height) {
    rhi_semaphore_wait(&gfx->frame_semaphore, gfx->current_frame - 1, -1);

    RHI_Texture *in_out_textures[RHI_MAX_BUFFER_COUNT] = {};
    u32 ptr = 0;

    for (u32 i = 0; i < gfx_backbuffer_count(); ++i) {
        in_out_textures[ptr++] = rhi_texture_from_guid(gfx->surface_guids[i]);
    }

    rhi_surface_resize(gfx->surface, width, height, in_out_textures);
}

static void gfx_init_uploader(u64 buffer_size) {
    // @Todo: Shutdown
    RHI_Buffer_Desc desc = {};
    desc.memory_type = RHI_MEMORY_UPLOAD;
    desc.size        = buffer_size;

    R_ASSERT(rhi_buffer_init(gfx->device, &gfx->upload_buffer, &desc, NULL));
    R_ASSERT(rhi_semaphore_init(gfx->device, &gfx->upload_semaphore));
    gfx->upload_semaphore_value = 1;

    gfx->upload_buffer_mapped = (u8*)rhi_buffer_map(&gfx->upload_buffer);
    gfx->upload_buffer_used   = 0;
}

u64 gfx_upload_reserve(u64 size, u64 alignment) {
    // @Cleanup: wtf?
    u64 capacity = gfx->upload_buffer.desc.size;
    R_ASSERT(size <= capacity);

    u64 offset = align_up(gfx->upload_buffer_used, alignment);

    // Out of room. Wait until every copy issued so far is done, then start over.
    if (offset + size > capacity) {
        rhi_semaphore_wait(&gfx->upload_semaphore, gfx->upload_semaphore_value - 1, -1);
        offset = 0;
    }

    gfx->upload_buffer_used = offset + size;
    return offset;
}

static void gfx_reset_per_frame_data() {
    // Reset per-frame data.
    gfx->context_pass = GFX_INVALID;

    // Pipeline
    gfx->context_pipeline = GFX_INVALID;
    gfx->next_pipeline    = 0;
    array_reset_keeping_memory(&gfx->pipelines);
    table_reset_keeping_memory(&gfx->pipeline_to_index_this_frame);

    // Push constants
    gfx->context_push_constants = GFX_INVALID;
    gfx->next_push_constants    = 0;
    array_reset_keeping_memory(&gfx->push_constants);
    table_reset_keeping_memory(&gfx->push_constants_to_index_this_frame);

    // Sort key and commands
    for (u32 i = 0; i < array_count(gfx->sort_keys); ++i) {
        array_reset_keeping_memory(&gfx->sort_keys[i]);
        array_reset_keeping_memory(&gfx->commands[i]);
    }

    // Frame graph
    for (int i = 0; i < array_count(gfx->out_edges); ++i) {
        array_reset_keeping_memory(&gfx->out_edges[i]);
    }

    // Pass states
    memset(&gfx->pass_states[0], 0, sizeof(gfx->pass_states));
}

static void gfx_add_callback(GFX_Callback_Entry entry)
{
    queue_push(&gfx->callbacks, entry);
}

static void gfx_execute_callbacks(u64 completed_value)
{
    // The values MUST be in the ascending order in the queue.

    while (gfx->callbacks.count > 0)
    {
        GFX_Callback_Entry entry = queue_front(&gfx->callbacks);
        if (entry.semaphore_value_to_execute > completed_value)  break;

        entry.proc(entry);
        queue_pop(&gfx->callbacks);
    }
}

void gfx_init(GFX_Info info, u32 num_backbuffers) {
    { // Alloc, Construct
        Allocator arena = arena_allocator_alloc();
        gfx = (GFX_State *)alloc(sizeof(GFX_State), arena);

        Construct(gfx);

        gfx->arena = arena; 
        gfx->heap  = { crt_proc, nullptr };
    }

    gfx->info = info;

    R_ASSERT(num_backbuffers >= RHI_MIN_BUFFER_COUNT && 
             num_backbuffers <= RHI_MAX_BUFFER_COUNT);

    // Allocate memory and initialize RHI
    gfx->device = (RHI_Device *)alloc(sizeof(RHI_Device), gfx->arena);
    R_ASSERT(rhi_device_init(gfx->device, info.kind, info.debug, info.break_on_warning, gfx->heap));


    { // Assign allocators
        gfx->callbacks.array.allocator = gfx->heap;

        gfx->pipelines.allocator = gfx->heap;
        gfx->pipeline_to_index_this_frame.allocator = gfx->heap;

        gfx->push_constants.allocator = gfx->heap;
        gfx->push_constants_to_index_this_frame.allocator = gfx->heap;

        for (u32 i = 0; i < array_count(gfx->sort_keys); ++i) {
            gfx->sort_keys[i].allocator = gfx->heap;
            gfx->commands[i].allocator  = gfx->heap;
        }

        gfx->mesh_table.allocator     = gfx->heap;
        gfx->texture_table.allocator  = gfx->heap;
        gfx->pipeline_table.allocator = gfx->heap;

        for (u32 i = 0; i < array_count(gfx->out_edges); ++i) {
            gfx->out_edges[i].allocator = gfx->heap;
        }
    }


    // Create swapchain
    {
        RHI_Texture *out_swapchain_textures[RHI_MAX_BUFFER_COUNT] = {};

        // Register entries to the table and fill in the pointers to the RHI_Texture to be filled.
        for (u32 i = 0; i < num_backbuffers; ++i) {
            Guid guid = guid_generate();
            gfx->surface_guids[i] = guid;
            GFX_Texture_Entry *entry = table_add(&gfx->texture_table, guid, {});
            out_swapchain_textures[i] = &entry->texture;
        }

        // Init swapchain and get textures
        gfx->surface = (RHI_Surface *)alloc(sizeof(RHI_Surface), gfx->arena);
        gfx_swapchain_init(info, num_backbuffers, out_swapchain_textures);
    }


    // Initialize frame semaphore
    rhi_semaphore_init(gfx->device, &gfx->frame_semaphore);


    // Initialize samplers
    gfx_init_samplers();


    // Fill in each bitfield's offset in sortkey
    for (u16 i = 0; i < GFX_KEY_COUNT - 1; i += 1) {
        gfx_key_offsets[i + 1] = gfx_key_offsets[i] + gfx_key_lengths[i];
    }


    // @Temporary: These will go to encoding threads.
    for (u32 i = 0; i < RHI_MAX_BUFFER_COUNT; ++i) {
        R_ASSERT(rhi_command_buffer_init(gfx->device, &gfx->command_buffers[i], RHI_COMMAND_TYPE_GRAPHICS));
        R_ASSERT(rhi_command_buffer_init(gfx->device, &gfx->compute_buffers[i], RHI_COMMAND_TYPE_COMPUTE));
    }

    R_ASSERT(rhi_command_buffer_init(gfx->device, &gfx->copy_buffer, RHI_COMMAND_TYPE_TRANSFER));


    gfx_init_uploader(Megabytes(128)); // @Temporary
}

void gfx_shutdown() {
    // Wait until all pending frames are finished.
    rhi_semaphore_wait(&gfx->frame_semaphore, gfx->current_frame - 1, -1);

    // @Temporary: These will go to encoding threads.
    for (u32 i = 0; i < RHI_MAX_BUFFER_COUNT; ++i) {
        rhi_command_buffer_deinit(&gfx->command_buffers[i]);
        rhi_command_buffer_deinit(&gfx->compute_buffers[i]);
    }

    rhi_command_buffer_deinit(&gfx->copy_buffer);

    // @Todo: release resources safely..

    gfx_deinit_samplers();
    rhi_semaphore_deinit(&gfx->frame_semaphore);
    gfx_swapchain_deinit();
    rhi_device_deinit(gfx->device);

    destroy(gfx->heap);
    destroy(gfx->arena);
}

static void gfx_create_gpu_buffer(RHI_Buffer *buffer, u32 size)
{
    RHI_Buffer_Desc desc = {};
    desc.memory_type = RHI_MEMORY_GPU_ONLY;
    desc.size        = size;

    R_ASSERT(rhi_buffer_init(gfx->device, buffer, &desc, NULL));
}

static void gfx_create_structured_view(RHI_Buffer_View *view, RHI_Buffer *buffer, u32 count, u32 stride)
{
    RHI_Buffer_View_Desc desc = {};
    {
        desc.type     = RHI_BUFFER_VIEW_TYPE_STRUCTURED;
        desc.writable = false;
        desc.stride   = stride;
        desc.offset   = 0;
        desc.size     = stride * count;
    }

    rhi_buffer_view_init(gfx->device, view, buffer, &desc);
}

static u64 gfx_encode_key(const u64 *subkeys) {
    u64 key = 0;

    for (u16 i = 0; i < GFX_KEY_COUNT; ++i) {
        u64 subkey = subkeys[i];
        R_ASSERT(subkey < (1ull << gfx_key_lengths[i]));
        key |= (subkey << gfx_key_offsets[i]);
    }

    return key;
}

static void gfx_decode_key(u64 key, u64 *out_subkeys) {
    for (u16 i = 0; i < GFX_KEY_COUNT; ++i) {
        u64 offset = gfx_key_offsets[i];
        u64 length = gfx_key_lengths[i];

        u64 mask   = ((1ull << length) - 1) << offset;
        u64 subkey = (key & mask) >> offset;
        out_subkeys[i] = subkey;
    }
}

void gfx_mesh_create(Guid guid, 
                     void *vertices, u32 num_vertices, u32 vertex_size, 
                     void *indices, u32 num_indices, u32 index_size)
{
    RHI_Buffer vb        = {};
    RHI_Buffer ib        = {};
    RHI_Buffer_View view = {};

    gfx_create_gpu_buffer(&vb, num_vertices * vertex_size);
    gfx_create_gpu_buffer(&ib, num_indices * index_size);
    gfx_create_structured_view(&view, &vb, num_vertices, vertex_size);

    GFX_Mesh entry = {};
    entry.vertex_buffer      = vb;
    entry.vertex_buffer_view = view;
    entry.index_buffer       = ib;
    entry.index_size         = index_size;
    entry.num_indices        = num_indices;

    table_add(&gfx->mesh_table, guid, entry);

    { // Upload vertex buffer and index buffer.
        u64 vb_sz = num_vertices * vertex_size;
        u64 ib_sz = num_indices * index_size;
        u64 total = vb_sz + ib_sz;

        u64 offset = gfx_upload_reserve(total, 16);
        u8 *dst = gfx->upload_buffer_mapped + offset;

        memcpy(dst,         vertices, vb_sz);
        memcpy(dst + vb_sz, indices,  ib_sz);

        rhi_command_buffer_begin(&gfx->copy_buffer);
        {
            rhi_cmd_copy_buffer_to_buffer(&gfx->copy_buffer, &vb, &gfx->upload_buffer, 0, offset,         vb_sz);
            rhi_cmd_copy_buffer_to_buffer(&gfx->copy_buffer, &ib, &gfx->upload_buffer, 0, offset + vb_sz, ib_sz);
        }
        rhi_command_buffer_end(&gfx->copy_buffer);
        RHI_Command_Buffer *buffers[] = {&gfx->copy_buffer};
        rhi_submit(gfx->device, 1, buffers);
        rhi_semaphore_signal(gfx->device, RHI_COMMAND_TYPE_TRANSFER, &gfx->upload_semaphore, gfx->upload_semaphore_value++);
    }
}

static void gfx_mesh_destroy_callback(GFX_Callback_Entry entry)
{
    GFX_Mesh *mesh = table_find_pointer(&gfx->mesh_table, entry.mesh_id);
    if (mesh)
    {
        rhi_buffer_view_deinit(&mesh->vertex_buffer_view);
        rhi_buffer_deinit(&mesh->vertex_buffer);
        rhi_buffer_deinit(&mesh->index_buffer);

        table_remove(&gfx->mesh_table, entry.mesh_id);
    }
}

void gfx_mesh_destroy(Guid guid) {
    // Destruction is deferred until the current workload on the GPU is done.
    // @Todo: This isn't settled yet.. Async asset loading, sorting pass, after-frame 
    // callbacks,...
    GFX_Callback_Entry entry = {};
    entry.semaphore_value_to_execute = gfx->current_frame;
    entry.proc    = gfx_mesh_destroy_callback;
    entry.mesh_id = guid;

    gfx_add_callback(entry);
}

void gfx_texture_create(Guid guid, RHI_Texture_Desc desc) {
    GFX_Texture_Entry entry = {};

    R_ASSERT( rhi_texture_init(gfx->device, &entry.texture, &desc, NULL) );

    auto CreateView = [&](RHI_Texture_View *view, RHI_Texture_View_Type type) {
        RHI_Texture_View_Desc vdesc = {};
        vdesc.type               = type;
        vdesc.dimension          = desc.type;
        vdesc.format             = desc.format;
        vdesc.base_mip_level     = 0;
        vdesc.base_array_layer   = 0;
        vdesc.mip_levels         = desc.mip_levels;
        vdesc.depth              = desc.depth;

        rhi_texture_view_init(gfx->device, view, &entry.texture, &vdesc);
    };

    if (desc.usage & RHI_TEXTURE_USAGE_SAMPLED) {
        CreateView(&entry.srv, RHI_TEXTURE_VIEW_TYPE_SAMPLED);
    }

    if (desc.usage & RHI_TEXTURE_USAGE_STORAGE) {
        CreateView(&entry.uav, RHI_TEXTURE_VIEW_TYPE_UNORDERED_ACCESS);
    }

    table_add(&gfx->texture_table, guid, entry);
}

void gfx_texture_destroy(Guid guid) {
    GFX_Texture_Entry *entry = table_find_pointer(&gfx->texture_table, guid);
    if (entry) {
        if (entry->srv.kind != RHI_KIND_INVALID) {
            rhi_texture_view_deinit(&entry->srv);
        }

        if (entry->srv.kind != RHI_KIND_INVALID) {
            rhi_texture_view_deinit(&entry->uav);
        }

        rhi_texture_deinit(&entry->texture);
        table_remove(&gfx->texture_table, guid);
    }
}

void gfx_texture_upload(Guid guid, RHI_Format format, void *data, u32 size, u32 width, u32 height) {
    GFX_Texture_Entry *tex = table_find_pointer(&gfx->texture_table, guid);

    if (tex) {
        // @Todo: I know, I know. Correct alignment for BC and other formats and
        // RHI abstraction of D3D12 and Vulkan. Those must be resolved...
        u32 pitch = size / height;
        u32 aligned_pitch = align_up(pitch, 256);
        u64 total = height * aligned_pitch;

        u64 offset = gfx_upload_reserve(total, 512); // @Temporary: FUCKING HATE ALIGNMENT
        u8 *dst = gfx->upload_buffer_mapped + offset;
        u8 *src = (u8 *)data;

        for (u32 r = 0; r < height; ++r) {
            memcpy(dst, src, pitch);
            dst += aligned_pitch;
            src += pitch;
        }

        RHI_Box box = {};
        box.width  = width;
        box.height = height;
        box.depth  = 1;

        rhi_command_buffer_begin(&gfx->copy_buffer);
        {
            rhi_cmd_copy_buffer_to_texture(&gfx->copy_buffer, &gfx->upload_buffer, offset, aligned_pitch, &tex->texture, &box, 0, 0);
        }
        rhi_command_buffer_end(&gfx->copy_buffer);
        RHI_Command_Buffer *buffers[] = {&gfx->copy_buffer};
        rhi_submit(gfx->device, 1, buffers);
        rhi_semaphore_signal(gfx->device, RHI_COMMAND_TYPE_TRANSFER, &gfx->upload_semaphore, gfx->upload_semaphore_value++);
    }
}

RHI_Texture_View *gfx_srv_from_texture(Guid guid) {
    GFX_Texture_Entry *entry = table_find_pointer(&gfx->texture_table, guid);
    if (entry && entry->srv.kind != RHI_KIND_INVALID)  return &entry->srv;
    return nullptr;
}

RHI_Texture_View *gfx_uav_from_texture(Guid guid) {
    GFX_Texture_Entry *entry = table_find_pointer(&gfx->texture_table, guid);
    if (entry && entry->uav.kind != RHI_KIND_INVALID)  return &entry->uav;
    return nullptr;
}

u32 gfx_srv_bindless_from_texture(Guid guid) {
    if (auto *view = gfx_srv_from_texture(guid))  return view->bindless;
    return GFX_INVALID_BINDLESS;
}

u32 gfx_uav_bindless_from_texture(Guid guid) {
    if (auto *view = gfx_uav_from_texture(guid))  return view->bindless;
    return GFX_INVALID_BINDLESS;
}

void gfx_pass_begin(u32 pass_index, GFX_Pass *pass) {
    R_ASSERT(pass_index < GFX_MAX_PASS);
    gfx->context_pass = pass_index;

    memcpy(&gfx->pass_states[pass_index], pass, sizeof(GFX_Pass));
}

void gfx_pass_end() {
    gfx->context_pass = GFX_INVALID;
}

void gfx_pass_connect(Guid resource,
                      u32 src_pass, 
                      u32 dst_pass, 
                      RHI_Resource_State dst_state) {
    R_ASSERT(resource != NULL_GUID);
    R_ASSERT( (src_pass == -1) || (src_pass < GFX_MAX_PASS && dst_pass < GFX_MAX_PASS) );

    GFX_Edge edge = {};
    edge.resource  = resource;
    edge.dst_pass  = dst_pass;
    edge.dst_state = dst_state;

    if (src_pass == -1)  src_pass = GFX_MAX_PASS; // nil

    array_add(&gfx->out_edges[src_pass], edge);
}

u32 gfx_backbuffer_index() {
    return gfx->surface->current_frame_index;
}

Guid gfx_surface_texture() {
    return gfx->surface_guids[gfx_backbuffer_index()];
}

u32 gfx_backbuffer_count(){
    return gfx->surface->desc.num_back_buffers;
}

RHI_Format gfx_surface_format() {
    return rhi_texture_from_guid(gfx->surface_guids[0])->desc.format;
}

void gfx_pipeline_create(Guid guid, RHI_Pipeline_Desc desc) {
    R_ASSERT(guid != NULL_GUID);
    R_ASSERT(gfx->pipeline_table.count <= GFX_MAX_PIPELINES);

    GFX_Pipeline_Entry entry = {};
    R_ASSERT(rhi_pipeline_init(gfx->device, &entry.rhi_pipeline, &desc));

    table_add(&gfx->pipeline_table, guid, entry);
}

void gfx_pipeline_destroy(Guid guid) {
    auto *entry = table_find_pointer(&gfx->pipeline_table, guid);
    if (entry) {
        rhi_pipeline_deinit(&entry->rhi_pipeline);
        table_remove(&gfx->pipeline_table, guid);
    }
}

void gfx_set_pipeline(Guid guid) {
    u64 index = GFX_INVALID;
    u64 *ptr = table_find_pointer(&gfx->pipeline_to_index_this_frame, guid);
    if (!ptr) {
        table_add(&gfx->pipeline_to_index_this_frame, guid, gfx->next_pipeline);
        index = gfx->next_pipeline;
        gfx->next_pipeline += 1;
        array_add(&gfx->pipelines, guid);
    } else {
        index = *ptr;
    }

    R_ASSERT(index < GFX_MAX_PIPELINES);

    gfx->context_pipeline = index;
}

void gfx_push_constants(void *data, u32 size) {
    R_ASSERT(size <= (4 * RHI_MAX_32BIT_PUSH_CONSTANTS));

    // Hash and check if the constant was added this frame.
    u64 index = GFX_INVALID;
    u64 hash = XXH3_64bits_withSeed(data, size, 0);
    u64 *ptr = table_find_pointer(&gfx->push_constants_to_index_this_frame, hash);

    if (!ptr) {
        // Acquire index
        table_add(&gfx->push_constants_to_index_this_frame, hash, gfx->next_push_constants);
        index = gfx->next_push_constants;
        gfx->next_push_constants += 1;

        // Add constants to the array
        array_add(&gfx->push_constants, {});
        auto *arr = &gfx->push_constants;
        GFX_Push_Constants *entry = &arr->data[arr->count - 1];
        u32 *dst = &entry->data[0];
        memcpy(dst, data, size);
        entry->size = size;
    } else {
        index = *ptr;
    }

    R_ASSERT(index < GFX_MAX_PUSH_CONSTANTS);

    gfx->context_push_constants = index;
}

void gfx_draw(Guid mesh_id, u32 num_instances) {
    R_ASSERT(gfx->context_pass != GFX_INVALID);

    auto *mesh = table_find_pointer(&gfx->mesh_table, mesh_id);

    if (mesh) {
        u64 subkeys[GFX_KEY_COUNT]  = {};
        subkeys[GFX_KEY_PIPELINE]   = gfx->context_pipeline;
        subkeys[GFX_KEY_CONSTANTS]  = gfx->context_push_constants;

        u32 pass = gfx->context_pass;

        GFX_Sort_Key key = {};
        key.bits      = gfx_encode_key(subkeys);
        key.cmd_index = gfx->commands[pass].count;

        array_add(&gfx->sort_keys[pass], key);

        GFX_Command cmd = {};
        cmd.mesh_handle         = mesh_id;
        cmd.num_instances       = num_instances;

        array_add(&gfx->commands[pass], cmd);
    } else if (gfx->info.debug) {
        log(LOG_WARNING, S("Draw attempted with an unregistered mesh."));
    }
}

static RHI_Pass rhi_pass_from_gfx(GFX_Pass *pass)
{
    RHI_Pass result = {};

    // Set name
    result.name = pass->name;

    // Allocate RTV or DSV.
    auto AllocViewAndFillAttachment = [](Guid guid, RHI_Texture_View_Type type, RHI_Attachment *attachment) ->RHI_Texture_View {
        GFX_Texture_Entry *entry = table_find_pointer(&gfx->texture_table, guid);
        R_ASSERT(entry);

        RHI_Texture *texture = &entry->texture;
        RHI_Texture_View view = {};

        RHI_Texture_View_Desc desc = {};
        desc.type               = type;
        desc.dimension          = texture->desc.type;
        desc.format             = texture->desc.format;
        desc.base_mip_level     = 0;
        desc.base_array_layer   = 0;
        desc.mip_levels         = texture->desc.mip_levels;
        desc.depth              = texture->desc.depth;

        rhi_texture_view_init(gfx->device, &view, texture, &desc);

        attachment->view     = view;
        attachment->load_op  = texture->desc.clear ? RHI_LOAD_OP_CLEAR : RHI_LOAD_OP_LOAD;
        attachment->store_op = RHI_STORE_OP_STORE;

        if (type == RHI_TEXTURE_VIEW_TYPE_RENDER_TARGET) {
            memcpy(attachment->clear_color, texture->desc.clear_color, sizeof(attachment->clear_color));
        } else if (type == RHI_TEXTURE_VIEW_TYPE_DEPTH_STENCIL) {
            attachment->clear_depth = texture->desc.clear_depth;
        } else {
            R_ASSERT(!"Invalid code path");
        }

        return view;
    };

    // As I'm regarding RTV and DSV as transient, release them after this frame.
    auto DeferReleaseViewAfterFrame = [](RHI_Texture_View view) {
        GFX_Callback_Entry callback = {};
        callback.semaphore_value_to_execute = gfx->current_frame;
        callback.view = view;
        callback.proc = [](GFX_Callback_Entry CE) {
            rhi_texture_view_deinit(&CE.view);
        };
        gfx_add_callback(callback);
    };

    // Determine if the pass has depth attachment and set it
    if (pass->depth_attachment != NULL_GUID) {
        result.has_depth_attachment = true;
        RHI_Texture_View dsv = AllocViewAndFillAttachment(pass->depth_attachment, 
                                                          RHI_TEXTURE_VIEW_TYPE_DEPTH_STENCIL, 
                                                          &result.depth_attachment);
        DeferReleaseViewAfterFrame(dsv);
    }

    // Compute number of color attachments and fill in
    for (u32 i = 0; i < RHI_MAX_COLOR_ATTACHMENTS; ++i) {
        if (pass->color_attachments[i] != NULL_GUID) {
            result.num_color_attachments += 1;
            RHI_Texture_View rtv = AllocViewAndFillAttachment(pass->color_attachments[i], 
                                                              RHI_TEXTURE_VIEW_TYPE_RENDER_TARGET,
                                                              &result.color_attachments[i]);
            DeferReleaseViewAfterFrame(rtv);
        }
    }

    return result;
}

/* Topologically sort frame graph and return array of passes in resolved order. */
static Array<u32> gfx_sort_frame_graph()
{
    const u32 N = array_count(gfx->out_edges);

    Array<u32> result = {};
    result.allocator = tctx.temp;
    array_reserve(&result, N);
    memset(result.data, 0xff, sizeof(result.data[0]) * N); // For debugging purpose

    u32 in_degree[N] = {};

    Queue<u32> q = {};
    q.array.allocator = tctx.temp;

    // Compute in-degree.
    for (u32 src = 0; src < N; ++src) {
        auto *edges = &gfx->out_edges[src];

        for (int e = 0; e < edges->count; ++e) {
            GFX_Edge *edge = &edges->data[e];
            in_degree[edge->dst_pass] += 1;
        }
    }

    // Push NIL
    queue_push(&q, (u32)GFX_MAX_PASS);

    // Resolve with Kahn's
    while (q.count != 0) {
        u32 p = queue_front(&q);
        queue_pop(&q);

        array_add(&result, p);

        for (GFX_Edge &edge : gfx->out_edges[p]) {
            in_degree[edge.dst_pass] -= 1;
            if (in_degree[edge.dst_pass] == 0) {
                queue_push(&q, edge.dst_pass);
            }
        }
    }

    return result;
}

void gfx_begin()
{
    // Resize swapchain if requested
    if (gfx->resize_requested) {
        gfx_swapchain_resize(gfx->resize_width, gfx->resize_height);
        gfx->resize_requested = false;

        gfx->info.width  = gfx->resize_width;
        gfx->info.height = gfx->resize_height;
    }
}

void gfx_end(f64 time, u32 sync_interval)
{
    ProfileScope;

    // Update shader time
    gfx->time = time; // @Todo: do dt trick from Witness.

    RHI_Command_Buffer *cmd_buffer = &gfx->command_buffers[gfx_backbuffer_index()];

    // Wait on frame semaphore.
    if (gfx->current_frame > gfx_backbuffer_count()) {
        rhi_semaphore_wait(&gfx->frame_semaphore, gfx->current_frame - gfx_backbuffer_count(), -1);
    }


    // Wait on resource upload semaphore.
    u64 value_to_wait = gfx->upload_semaphore_value - 1;
    rhi_queue_wait(gfx->device, RHI_COMMAND_TYPE_GRAPHICS, &gfx->upload_semaphore, value_to_wait);
    rhi_queue_wait(gfx->device, RHI_COMMAND_TYPE_COMPUTE,  &gfx->upload_semaphore, value_to_wait);
    gfx->upload_buffer_used = 0;


    // Sort frame graph dependencies
    Array<u32> passes_in_order = gfx_sort_frame_graph();


    // Sort keys
    for (u32 i = 0; i < GFX_MAX_PASS; ++i) {
        radix_sort_u64(gfx->sort_keys[i].data,
                       gfx->sort_keys[i].count, 
                       sizeof(GFX_Sort_Key),
                       offset_of(GFX_Sort_Key, bits));
    }

    // @Temporary
    rhi_command_buffer_begin(cmd_buffer);
    {
        // Execute passes in sorted order
        for (u32 pass_id : passes_in_order)
        {
            // If it's NIL, just go set barriers
            if (pass_id != GFX_MAX_PASS) 
            {
                // Translate pass
                GFX_Pass *gfx_pass = &gfx->pass_states[pass_id];
                RHI_Pass pass   = rhi_pass_from_gfx(gfx_pass);
                GFX_Viewport vp = gfx_pass->viewport;
                GFX_Scissor sc  = gfx_pass->scissor;


                // Begin pass
                rhi_pass_begin(cmd_buffer, &pass);
                rhi_cmd_set_viewport(cmd_buffer, vp.x, vp.y, vp.w, vp.h, gfx_pass->min_depth, gfx_pass->max_depth);
                rhi_cmd_set_scissor(cmd_buffer, sc.x, sc.y, sc.w, sc.h);


                // Sort key states
                u64 current_keys[GFX_KEY_COUNT];
                for (u16 i = 0; i < GFX_KEY_COUNT; ++i)  current_keys[i] = GFX_INVALID;


                // For keys in the pass's bucket
                for (auto& key : gfx->sort_keys[pass_id]) 
                {
                    // Decode the key.
                    u64 keys[GFX_KEY_COUNT];
                    gfx_decode_key(key.bits, keys);

                    u64 pipeline_index      = keys[GFX_KEY_PIPELINE];
                    u64 push_constant_index = keys[GFX_KEY_CONSTANTS];


                    // Get the corresponding command.
                    GFX_Command cmd = gfx->commands[pass_id][key.cmd_index];


                    // Update pipeline?
                    if (current_keys[GFX_KEY_PIPELINE] != pipeline_index) {
                        current_keys[GFX_KEY_PIPELINE] = pipeline_index;

                        Guid pipeline_guid = gfx->pipelines[pipeline_index];
                        auto *entry = table_find_pointer(&gfx->pipeline_table, pipeline_guid);
                        R_ASSERT(entry);
                        rhi_cmd_set_pipeline(cmd_buffer, &entry->rhi_pipeline);
                    }


                    // @Fix: Blindly pushing constants
                    if (push_constant_index != GFX_INVALID) {
                        GFX_Push_Constants *constants = &gfx->push_constants.data[push_constant_index];
                        rhi_cmd_push_constants(cmd_buffer, GFX_CONSTANTS_INDEX_USER, constants->data, constants->size);
                    }


                    // Draw mesh.
                    auto *mesh = table_find_pointer(&gfx->mesh_table, cmd.mesh_handle);
                    if (mesh) {
                        rhi_cmd_draw_indexed(cmd_buffer, 
                                             &mesh->index_buffer, mesh->index_size, mesh->num_indices, 
                                             cmd.num_instances, 0, 0, 0);
                    }
                }

                // End pass
                rhi_pass_end(cmd_buffer, &pass);
            }

            // Install barriers for the next passses in the graph.
            auto *edges = &gfx->out_edges[pass_id];

            for (u32 i = 0; i < edges->count; ++i) {
                auto *edge = &edges->data[i];

                auto *tex = rhi_texture_from_guid(edge->resource);
                R_ASSERT(tex);

                if (tex) {
                    rhi_cmd_texture_barrier(cmd_buffer, tex, edge->dst_state, RHI_ALL_MIPS, RHI_ALL_LAYERS);
                } else {
                    R_ASSERT(!"Texture not found."); // @Todo: Error-handling
                }
            }

        } // for each pass

        rhi_cmd_texture_barrier(cmd_buffer, 
                                rhi_texture_from_guid(gfx->surface_guids[gfx_backbuffer_index()]),
                                RHI_RESOURCE_STATE_PRESENT, RHI_ALL_MIPS, RHI_ALL_LAYERS);
    }
    rhi_command_buffer_end(cmd_buffer);


    // Submit command buffer
    RHI_Command_Buffer *buffers[] = { cmd_buffer };
    rhi_submit(gfx->device, 1, buffers);


    // @Study: Present and Signal ordering...
    rhi_surface_present(gfx->surface, sync_interval);
    rhi_semaphore_signal(gfx->device, RHI_COMMAND_TYPE_GRAPHICS, &gfx->frame_semaphore, gfx->current_frame);


    // Clear per-frame data
    gfx_reset_per_frame_data();


    u64 completed_value = rhi_semaphore_completed_value(&gfx->frame_semaphore);
    gfx_execute_callbacks(completed_value);

    gfx->current_frame += 1;
}

bool gfx_wait_for_frame_waitable_object() {
    ProfileScope;
    return rhi_surface_wait_for_waitable_object(gfx->surface);
}

void gfx_request_swapchain_resize(u32 width, u32 height) {
    gfx->resize_requested = true;
    gfx->resize_width     = width;
    gfx->resize_height    = height;
}
