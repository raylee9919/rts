// Copyright Seong Woo Lee. All Rights Reserved.


static void gfx_create_swapchain_depth_textures(u32 width, u32 height) {
    RHI_Texture_Desc desc = {}; 
    desc.type           = RHI_TEXTURE_TYPE_2D;
    desc.format         = RHI_FORMAT_D32F;
    desc.usage          = RHI_TEXTURE_USAGE_DEPTH_STENCIL_ATTACHMENT;
    desc.width          = width;
    desc.height         = height;
    desc.mip_levels     = 1;
    desc.depth          = 1;
    desc.clear          = true;
    desc.clear_depth    = 1.f;

    for (int i = 0; i < RHI_MAX_BUFFER_COUNT; ++i) {
        gfx->depth_textures[i] = guid_generate();
        gfx_texture_create(gfx->depth_textures[i], desc);
    }
}

static void gfx_destroy_swapchain_depth_textures() {
    for (int i = 0; i < RHI_MAX_BUFFER_COUNT; ++i) {
        gfx_texture_destroy(gfx->depth_textures[i]);
    }
}

static void gfx_init_samplers() {
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

static void gfx_deinit_samplers() {
    rhi_sampler_deinit(&gfx->linear_sampler);
}

static void gfx_init_swapchain(void *native_window_handle, u32 width, u32 height, u32 num_back_buffers) {
    RHI_Surface_Desc desc = {};
    desc.native_window_handle = native_window_handle;
    desc.width                = width;
    desc.height               = height;
    desc.num_back_buffers     = num_back_buffers;

    Assert(rhi_surface_init(gfx->device, gfx->surface, &desc));
}

static void gfx_deinit_swapchain() {
    // rhi_surface_deinit(gfx->surface);
}

static void gfx_init_swapchain_views() {
    for (u32 i = 0; i < RHI_MAX_BUFFER_COUNT; ++i)  {
        RHI_Texture_View_Desc desc = {};
        desc.type              = RHI_TEXTURE_VIEW_TYPE_RENDER_TARGET;
        desc.dimension         = RHI_TEXTURE_TYPE_2D;
        desc.format            = gfx->surface->textures[i].desc.format;
        desc.base_mip_level    = 0;
        rhi_texture_view_init(gfx->device, &gfx->surface_views[i], &gfx->surface->textures[i], &desc);
    }
}

static void gfx_resize_swapchain(u32 width, u32 height) {
    rhi_semaphore_wait(&gfx->frame_semaphore, gfx->current_frame - 1, -1);

    for (u32 i = 0; i < RHI_MAX_BUFFER_COUNT; ++i)  {
        rhi_texture_view_deinit(&gfx->surface_views[i]);
    }
    rhi_surface_resize(gfx->surface, width, height);
    gfx_init_swapchain_views();
    gfx_destroy_swapchain_depth_textures();
    gfx_create_swapchain_depth_textures(width, height);
}

static void gfx_init_uploader(u64 buffer_size)
{
    RHI_Buffer_Desc desc = {};
    desc.memory_type = RHI_MEMORY_UPLOAD;
    desc.size        = buffer_size;

    Assert(rhi_buffer_init(gfx->device, &gfx->upload_buffer, &desc, NULL));
    Assert(rhi_semaphore_init(gfx->device, &gfx->upload_semaphore));
    gfx->upload_semaphore_value = 1;

    Assert(rhi_command_buffer_init(gfx->device, &gfx->copy_buffer, RHI_COMMAND_TYPE_TRANSFER));
}

static void gfx_reset_per_frame_data() {
    // Reset per-frame data.
    gfx->current_pass           = GFX_PASS_NULL;

    // Pipeline
    gfx->context_pipeline = GFX_PIPELINE_INVALID;
    gfx->next_pipeline    = 0;
    array_reset_keeping_memory(&gfx->pipelines);
    table_reset_keeping_memory(&gfx->pipeline_to_index_this_frame);

    // Push constants
    gfx->current_push_constants = 0;
    array_reset_keeping_memory(&gfx->push_constants);


    array_reset_keeping_memory(&gfx->sort_keys);
    array_reset_keeping_memory(&gfx->commands);

    memset(&gfx->pass_states[0], 0, sizeof(gfx->pass_states[0]) * GFX_MAX_PASS);
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
        auto entry = queue_front(&gfx->callbacks);
        if (entry.semaphore_value_to_execute > completed_value)  break;

        entry.proc(entry);
        queue_pop(&gfx->callbacks);
    }
}

void gfx_init(GFX_Info info) {
    { // Alloc, Construct
        Allocator arena = arena_allocator_alloc();
        gfx = (GFX_State *)alloc(sizeof(GFX_State), arena);

        Construct(gfx);

        gfx->arena = arena; 
    }

    gfx->info = info;

    Assert(info.num_frames >= GFX_MIN_FRAME_COUNT && info.num_frames <= GFX_MAX_FRAME_COUNT);

    gfx->device = (RHI_Device *)alloc(sizeof(RHI_Device), gfx->arena);
    Assert(rhi_device_init(gfx->device, info.kind, info.debug, info.break_on_warning));

    gfx->surface = (RHI_Surface *)alloc(sizeof(RHI_Surface), gfx->arena);
    gfx_init_swapchain(info.native_window_handle, info.width, info.height, info.num_buffers);
    gfx_init_swapchain_views();
    gfx_create_swapchain_depth_textures(info.width, info.height);

    rhi_semaphore_init(gfx->device, &gfx->frame_semaphore);

    gfx_init_samplers();

    gfx_init_uploader(128ull * 1024 * 1024); // @Temporary

    // @Temporary: These will go to encoding threads.
    for (u32 i = 0; i < GFX_MAX_FRAME_COUNT; ++i) {
        Assert(rhi_command_buffer_init(gfx->device, &gfx->command_buffers[i], RHI_COMMAND_TYPE_GRAPHICS));
        Assert(rhi_command_buffer_init(gfx->device, &gfx->compute_buffers[i], RHI_COMMAND_TYPE_COMPUTE));
    }

    GFX_SURFACE_TEXTURE = guid_generate();

    gfx->initted = true;
}

void gfx_shutdown() {
    // Wait until all pending frames are finished.
    rhi_semaphore_wait(&gfx->frame_semaphore, gfx->current_frame - 1, -1);

    // @Temporary: These will go to encoding threads.
    for (u32 i = 0; i < GFX_MAX_FRAME_COUNT; ++i) {
        rhi_command_buffer_deinit(&gfx->command_buffers[i]);
        rhi_command_buffer_deinit(&gfx->compute_buffers[i]);
    }

    // @Todo: release resources safely..

    gfx_deinit_samplers();
    rhi_semaphore_deinit(&gfx->frame_semaphore);
    gfx_deinit_swapchain();
    rhi_device_deinit(gfx->device);

    release(gfx->arena);
}

static void gfx_create_gpu_buffer(RHI_Buffer *buffer, u32 size)
{
    RHI_Buffer_Desc desc = {};
    desc.memory_type = RHI_MEMORY_GPU_ONLY;
    desc.size        = size;

    Assert(rhi_buffer_init(gfx->device, buffer, &desc, NULL));
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
        Assert(subkey < (1ull << gfx_key_lengths[i]));
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

void gfx_mesh_create(Guid guid, void *vertices, u32 num_vertices, u32 vertex_size, void *indices, u32 num_indices, u32 index_size)
{
    RHI_Buffer vb        = {};
    RHI_Buffer ib        = {};
    RHI_Buffer_View view = {};

    gfx_create_gpu_buffer(&vb, num_vertices * vertex_size);
    gfx_create_gpu_buffer(&ib, num_indices * index_size);
    gfx_create_structured_view(&view, &vb, num_vertices, vertex_size);

    GFX_Mesh entry;
    entry.vertex_buffer      = vb;
    entry.vertex_buffer_view = view;
    entry.index_buffer       = ib;
    entry.index_size         = index_size;
    entry.num_indices        = num_indices;

    table_add(&gfx->mesh_table, guid, entry);

    { // @Temporary: Upload vertex buffer and index buffer.
        void *ptr = 0;
        ptr = rhi_buffer_map(&gfx->upload_buffer);

        u64 vb_sz = num_vertices * vertex_size;
        memcpy(ptr, vertices, vb_sz);

        u64 ib_sz = num_indices * index_size;
        memcpy((u8 *)ptr + vb_sz, indices, ib_sz);

        rhi_buffer_unmap(&gfx->upload_buffer);

        rhi_command_buffer_begin(&gfx->copy_buffer);
        {
            rhi_cmd_copy_buffer_to_buffer(&gfx->copy_buffer, &vb, &gfx->upload_buffer, 0, 0, vb_sz);
            rhi_cmd_copy_buffer_to_buffer(&gfx->copy_buffer, &ib, &gfx->upload_buffer, 0, vb_sz, ib_sz);
        }
        rhi_command_buffer_end(&gfx->copy_buffer);
        RHI_Command_Buffer *buffers[] = {&gfx->copy_buffer};
        rhi_submit(gfx->device, 1, buffers);
        rhi_semaphore_signal(gfx->device, RHI_COMMAND_TYPE_TRANSFER, &gfx->upload_semaphore, gfx->upload_semaphore_value);

        rhi_semaphore_wait(&gfx->upload_semaphore, gfx->upload_semaphore_value, -1);
        gfx->upload_semaphore_value += 1;
    }
}

static void gfx_mesh_destroy_callback(GFX_Callback_Entry entry)
{
    auto *mesh = table_find_pointer(&gfx->mesh_table, entry.mesh_id);
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

void gfx_material_alloc(Guid guid, GFX_Material material) {
    table_add(&gfx->material_table, guid, material);
}

void gfx_material_dealloc(Guid guid) {
    table_remove(&gfx->material_table, guid);
}

GFX_Material *gfx_material_pointer_from_guid(Guid guid) {
    GFX_Material *result = table_find_pointer(&gfx->material_table, guid);
    return result;
}

void gfx_texture_create(Guid guid, RHI_Texture_Desc desc) {
    GFX_Texture_Entry entry = {};

    Assert( rhi_texture_init(gfx->device, &entry.texture, &desc, NULL) );

    RHI_Texture_View_Type view_type = 0;

    for (u32 flag = 0x1; flag < RHI_TEXTURE_USAGE_OPL_BIT; flag <<= 1) {
        if (desc.usage & flag) { 
            RHI_Texture_View_Desc vdesc = {};
            vdesc.type               = view_type;
            vdesc.dimension          = desc.type;
            vdesc.format             = desc.format;
            vdesc.base_mip_level     = 0;
            vdesc.base_array_layer   = 0;
            vdesc.mip_levels         = desc.mip_levels;
            vdesc.depth              = desc.depth;

            rhi_texture_view_init(gfx->device, &entry.views[view_type], &entry.texture, &vdesc);
        }

        view_type += 1;
    }

    table_add(&gfx->texture_table, guid, entry);
}

void gfx_texture_destroy(Guid guid) {
    auto *entry = table_find_pointer(&gfx->texture_table, guid);
    if (entry) {
        for (u16 i = 0; i < RHI_TEXTURE_VIEW_TYPE_COUNT; ++i) {
            if (entry->views[i].kind != RHI_KIND_INVALID) {
                rhi_texture_view_deinit(&entry->views[i]);
            }
        }
        rhi_texture_deinit(&entry->texture);
        
        table_remove(&gfx->texture_table, guid);
    }
}

void gfx_texture_upload(Guid guid, RHI_Format format, void *data, u32 size, u32 width, u32 height) {
    auto *tex = table_find_pointer(&gfx->texture_table, guid);

    if (tex) {
        // @Todo: I know, I know. Correct alignment for BC and other formats and 
        // RHI abstraction of D3D12 and Vulkan. Those must be resolved...
        u8 *ptr = (u8 *)rhi_buffer_map(&gfx->upload_buffer);
        u8 *dst = ptr;
        u8 *src = (u8 *)data;
        u32 pitch = size / height;
        u32 aligned_pitch = align_up(pitch, 256);

        for (u32 r = 0; r < height; ++r) {
            memcpy(dst, src, pitch);
            dst += aligned_pitch;
            src += pitch;
        }

        rhi_buffer_unmap(&gfx->upload_buffer);

        RHI_Box box = {};
        box.width  = width;
        box.height = height;
        box.depth  = 1;

        rhi_command_buffer_begin(&gfx->copy_buffer);
        {
            rhi_cmd_copy_buffer_to_texture(&gfx->copy_buffer, &gfx->upload_buffer, 0, aligned_pitch, &tex->texture, &box, 0, 0);
        }
        rhi_command_buffer_end(&gfx->copy_buffer);
        RHI_Command_Buffer *buffers[] = {&gfx->copy_buffer};
        rhi_submit(gfx->device, 1, buffers);
        rhi_semaphore_signal(gfx->device, RHI_COMMAND_TYPE_TRANSFER, &gfx->upload_semaphore, gfx->upload_semaphore_value);
        rhi_semaphore_wait(&gfx->upload_semaphore, gfx->upload_semaphore_value, -1);
        gfx->upload_semaphore_value += 1;
    }
}

u32 gfx_bindless_from_texture(Guid guid, RHI_Texture_View_Type view_type) {
    u32 id = GFX_INVALID_BINDLESS;
    GFX_Texture_Entry *entry = table_find_pointer(&gfx->texture_table, guid);
    if (entry) {
        id = entry->views[view_type].bindless;
    }
    return id;
}

void gfx_pass_begin(u32 pass_index) {
    Assert(pass_index < GFX_MAX_PASS);
    gfx->current_pass = pass_index;
}

void gfx_pass_end() {
    gfx->current_pass = GFX_PASS_NULL;
}

void gfx_pass_color_attachment(u32 pass_index, u32 color_attachment_index, Guid guid) {
    Assert(pass_index < GFX_MAX_PASS);

    auto *pass = &gfx->pass_states[pass_index];

    pass->color_attachments[color_attachment_index] = guid;
}

void gfx_pass_depth_attachment(u32 pass_index, Guid guid) {
    Assert(pass_index < GFX_MAX_PASS);

    auto *pass = &gfx->pass_states[pass_index];

    pass->depth_attachment = guid;
}

void gfx_set_viewport(f32 top_left_x, f32 top_left_y, f32 width, f32 height) {
    Assert(gfx->current_pass < GFX_MAX_PASS);

    auto *vp = &gfx->pass_states[gfx->current_pass].viewport;

    vp->set = true;
    vp->x = top_left_x;
    vp->y = top_left_y;
    vp->w = width;
    vp->h = height;
}

void gfx_set_scissor(u32 top_left_x, u32 top_left_y, u32 width, u32 height) {
    Assert(gfx->current_pass < GFX_MAX_PASS);

    auto *sc = &gfx->pass_states[gfx->current_pass].scissor;

    sc->set = true;
    sc->x = top_left_x;
    sc->y = top_left_y;
    sc->w = width;
    sc->h = height;
}

void gfx_pass_clear_color(u32 pass_index, u32 clear_color, u32 color_attachment_index)
{
    Assert(pass_index < GFX_MAX_PASS && color_attachment_index < RHI_MAX_COLOR_ATTACHMENTS);

    gfx->pass_states[pass_index].flags |= (1 << color_attachment_index);
    gfx->pass_states[pass_index].colors[color_attachment_index] = clear_color;
}

void gfx_pass_clear_depth(u32 pass_index, f32 clear_depth)
{
    Assert(pass_index < GFX_MAX_PASS);

    gfx->pass_states[pass_index].flags |= GFX_PASS_FLAG_CLEAR_DEPTH_STENCIL;
    gfx->pass_states[pass_index].depth  = clear_depth;
}

void gfx_pipeline_create(Guid guid, RHI_Pipeline_Desc desc) {
    Assert(gfx->pipeline_table.count <= GFX_MAX_PIPELINES);

    GFX_Pipeline_Entry entry = {};
    Assert(rhi_pipeline_init(gfx->device, &entry.rhi_pipeline, &desc));

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
    u64 index = GFX_PIPELINE_INVALID;
    u64 *ptr = table_find_pointer(&gfx->pipeline_to_index_this_frame, guid);
    if (!ptr) {
        table_add(&gfx->pipeline_to_index_this_frame, guid, gfx->next_pipeline);
        index = gfx->next_pipeline;
        gfx->next_pipeline += 1;
        array_add(&gfx->pipelines, guid);
    } else {
        index = *ptr;
    }

    Assert(index <= GFX_MAX_PIPELINES); // Technically, it's max # of pipelines per frame..

    gfx->context_pipeline = index;
}

void gfx_push_constants(void *data, u32 size) {
    Assert(size <= (4 * RHI_MAX_32BIT_PUSH_CONSTANTS));

    array_add(&gfx->push_constants, {});
    auto *arr = &gfx->push_constants;
    auto *entry = &arr->data[arr->count - 1];
    void *dst = &entry->data[0];
    memcpy(dst, data, size);

    entry->size = size;

    gfx->current_push_constants += 1;
}

void gfx_draw(Guid mesh_id, u32 num_instances) {
    Assert(gfx->current_pass != GFX_PASS_NULL);

    auto *mesh = table_find_pointer(&gfx->mesh_table, mesh_id);

    if (mesh) {
        u64 subkeys[GFX_KEY_COUNT]  = {};
        subkeys[GFX_KEY_PASS]       = gfx->current_pass;
        subkeys[GFX_KEY_PIPELINE]   = gfx->context_pipeline;
        subkeys[GFX_KEY_CONSTANTS]  = gfx->current_push_constants;

        GFX_Sort_Key key = {};
        key.bits      = gfx_encode_key(subkeys);
        key.cmd_index = gfx->commands.count;

        array_add(&gfx->sort_keys, key);

        GFX_Command cmd = {};
        cmd.pipeline_index      = gfx->context_pipeline;
        cmd.push_constant_index = gfx->current_push_constants;
        cmd.mesh_handle         = mesh_id;
        cmd.num_instances       = num_instances;

        array_add(&gfx->commands, cmd);
    } else if (gfx->info.debug) {
        log(LOG_WARNING, S("Draw attempted with an unregistered mesh."));
        Assert(0);
    }
}

static void gfx_begin_pass_and_set_viewport_scissor(u64 pass_idx, RHI_Pass *pass, RHI_Command_Buffer *cmd_buffer)
{
    auto *p = &gfx->pass_states[pass_idx];
    auto vp = p->viewport;
    auto sc = p->scissor;

    for (u32 i = 0; i < RHI_MAX_COLOR_ATTACHMENTS; ++i) {
        if (p->color_attachments[i] != NULL_GUID) {
            pass->num_color_attachments += 1;
        }
    }


    if (p->depth_attachment != NULL_GUID) {
        pass->has_depth_attachment = true;
    }

    for (u16 color_idx = 0; color_idx < RHI_MAX_COLOR_ATTACHMENTS; ++color_idx) {
        if (p->flags & (GFX_PASS_FLAG_CLEAR_COLOR_0 << (color_idx))) {
            Guid guid = p->color_attachments[color_idx];

            // @Cleanup: I don't like this additional code path.
            if (guid != GFX_SURFACE_TEXTURE) {
                auto *tex = table_find_pointer(&gfx->texture_table, guid);
                if (tex) {
                    auto *attachment = &pass->color_attachments[color_idx];
                    attachment->view = tex->views[RHI_TEXTURE_VIEW_TYPE_RENDER_TARGET];

                    attachment->load_op = RHI_LOAD_OP_CLEAR;

                    u32 c_packed = p->colors[color_idx];
                    v4 c_unpacked = unpack_rgba(c_packed);
                    memcpy(attachment->clear_color, &c_unpacked, sizeof(f32) * 4);
                } else {
                    // @Todo: Error-handling.
                    log(LOG_ERROR, S("Color attachment texture wasn't found."));
                    Assert(0);
                }
            } else {
                auto *attachment = &pass->color_attachments[color_idx];
                attachment->view = gfx->surface_views[gfx->surface->current_frame_index];

                attachment->load_op = RHI_LOAD_OP_CLEAR;

                u32 c_packed = p->colors[color_idx];
                v4 c_unpacked = unpack_rgba(c_packed);
                memcpy(attachment->clear_color, &c_unpacked, sizeof(f32) * 4);
            }
        }
    }

    if (p->flags & GFX_PASS_FLAG_CLEAR_DEPTH_STENCIL) {
        Guid guid = p->depth_attachment;
        auto *tex = table_find_pointer(&gfx->texture_table, guid);

        if (tex) {
            auto *attachment = &pass->depth_attachment;
            attachment->view        = tex->views[RHI_TEXTURE_VIEW_TYPE_DEPTH_STENCIL];;
            attachment->load_op     = RHI_LOAD_OP_CLEAR;
            attachment->clear_depth = p->depth;
        } else {
            // @Todo: Error-handling.
            log(LOG_ERROR, S("Depth attachment texture wasn't found."));
            Assert(0);
        }
    }

    rhi_pass_begin(cmd_buffer, pass);

    rhi_cmd_set_viewport(cmd_buffer, vp.x, vp.y, vp.w, vp.h, 0.f, 1.f);
    rhi_cmd_set_scissor(cmd_buffer, sc.x, sc.y, sc.w, sc.h);
}

void gfx_end(f64 time, u32 sync_interval)
{
    ProfileScopeN("gfx_end");

    // Resize swapchain if requested
    if (gfx->resize_requested) {
        gfx_resize_swapchain(gfx->resize_width, gfx->resize_height);
        gfx->resize_requested = false;
    }

    // Update shader time
    gfx->time = time; // @Todo: do dt trick from Witness.

    auto *cmd_buffer = &gfx->command_buffers[gfx->current_frame % gfx->info.num_frames];

    // Wait on frame semaphore.
    if (gfx->current_frame > gfx->info.num_frames) {
        rhi_semaphore_wait(&gfx->frame_semaphore, gfx->current_frame - gfx->info.num_frames, -1);
    }

    {
        ProfileScopeN("gfx_sort_keys");
        radix_sort_u64(gfx->sort_keys.data, gfx->sort_keys.count, sizeof(GFX_Sort_Key), offset_of(GFX_Sort_Key, bits));
    }

    // @Temporary
    rhi_command_buffer_begin(cmd_buffer);
    {
        RHI_Pass pass    = {};
        u64 current_pass = GFX_PASS_NULL;

        u64 current_pipeline = GFX_PIPELINE_INVALID;

        for (auto& key : gfx->sort_keys) {
            // @Temporary: Decode the key.
            u64 subkeys[GFX_KEY_COUNT];
            gfx_decode_key(key.bits, subkeys);

            u64 pass_idx            = subkeys[GFX_KEY_PASS];
            u64 pipeline            = subkeys[GFX_KEY_PIPELINE];
            u64 push_constant_index = subkeys[GFX_KEY_CONSTANTS];

            // Get the corresponding command.
            auto cmd = gfx->commands[key.cmd_index];

            // @Fix, Robustness, Todo: If pass changes, pipeline must be reset.
            // Automate barrier installation.
            //
            if (current_pass != pass_idx) {
                current_pipeline = GFX_PIPELINE_INVALID;

                if (current_pass != GFX_PASS_NULL) {
                    rhi_pass_end(cmd_buffer, &pass);
                    rhi_cmd_texture_barrier(cmd_buffer, &gfx->surface->textures[gfx->surface->current_frame_index], RHI_RESOURCE_STATE_RENDER_TARGET, RHI_RESOURCE_STATE_PRESENT, RHI_ALL_MIPS, RHI_ALL_LAYERS);

                    Guid depth_attachment_guid = gfx->pass_states[pass_idx].depth_attachment;
                    if (depth_attachment_guid != NULL_GUID) {
                        auto *entry = table_find_pointer(&gfx->texture_table, depth_attachment_guid);
                        if (entry) {
                            rhi_cmd_texture_barrier(cmd_buffer, &entry->texture, RHI_RESOURCE_STATE_DEPTH_WRITE, RHI_RESOURCE_STATE_COMMON, RHI_ALL_MIPS, RHI_ALL_LAYERS);
                        }
                    }
                }
                current_pass = pass_idx;

                rhi_cmd_texture_barrier(cmd_buffer, &gfx->surface->textures[gfx->surface->current_frame_index], RHI_RESOURCE_STATE_COMMON, RHI_RESOURCE_STATE_RENDER_TARGET, RHI_ALL_MIPS, RHI_ALL_LAYERS);

                Guid depth_attachment_guid = gfx->pass_states[pass_idx].depth_attachment;
                if (depth_attachment_guid != NULL_GUID) {
                    auto *entry = table_find_pointer(&gfx->texture_table, depth_attachment_guid);
                    if (entry) {
                        rhi_cmd_texture_barrier(cmd_buffer, &entry->texture, RHI_RESOURCE_STATE_COMMON, RHI_RESOURCE_STATE_DEPTH_WRITE, RHI_ALL_MIPS, RHI_ALL_LAYERS);
                    }
                }

                gfx_begin_pass_and_set_viewport_scissor(pass_idx, &pass, cmd_buffer);

            }

            // Update pipeline?
            if (current_pipeline != cmd.pipeline_index) {
                current_pipeline = cmd.pipeline_index;
                Guid pipeline_guid = gfx->pipelines[cmd.pipeline_index];
                auto *entry = table_find_pointer(&gfx->pipeline_table, pipeline_guid);
                Assert(entry);
                rhi_cmd_set_pipeline(cmd_buffer, &entry->rhi_pipeline);


                // Push shader global data
                {
                    GPU_Global g = {};
                    g.time = gfx->time;

                    rhi_cmd_push_constants(cmd_buffer, GFX_CONSTANTS_INDEX_GLOBAL, &g, sizeof(g));
                }
            }

            // @Fix: Duplicate?
            // And the real push constants.
            if (cmd.push_constant_index != 0) {
                auto* entry = &gfx->push_constants.data[cmd.push_constant_index - 1];
                rhi_cmd_push_constants(cmd_buffer, GFX_CONSTANTS_INDEX_USER, entry->data, entry->size);
            }

            // Draw mesh.
            auto *mesh = table_find_pointer(&gfx->mesh_table, cmd.mesh_handle);
            if (mesh) {
                rhi_cmd_draw_indexed(cmd_buffer, &mesh->index_buffer, mesh->index_size, mesh->num_indices, cmd.num_instances, 0, 0, 0);
            }
        }

        if (current_pass != GFX_PASS_NULL) {
            rhi_pass_end(cmd_buffer, &pass);
            rhi_cmd_texture_barrier(cmd_buffer, &gfx->surface->textures[gfx->surface->current_frame_index], RHI_RESOURCE_STATE_RENDER_TARGET, RHI_RESOURCE_STATE_PRESENT, RHI_ALL_MIPS, RHI_ALL_LAYERS);
            
            Guid depth_attachment_guid = gfx->pass_states[current_pass].depth_attachment;
            if (depth_attachment_guid != NULL_GUID) {
                auto *entry = table_find_pointer(&gfx->texture_table, depth_attachment_guid);
                if (entry) {
                    rhi_cmd_texture_barrier(cmd_buffer, &entry->texture, RHI_RESOURCE_STATE_DEPTH_WRITE, RHI_RESOURCE_STATE_COMMON, RHI_ALL_MIPS, RHI_ALL_LAYERS);
                }
            }
        }
    }
    rhi_command_buffer_end(cmd_buffer);

    RHI_Command_Buffer *buffers[] = { cmd_buffer };
    rhi_submit(gfx->device, 1, buffers);

    // @Study: Present and Signal ordering...
    rhi_surface_present(gfx->surface, sync_interval);
    rhi_semaphore_signal(gfx->device, RHI_COMMAND_TYPE_GRAPHICS, &gfx->frame_semaphore, gfx->current_frame);

    gfx_reset_per_frame_data();

    u64 completed_value = rhi_semaphore_completed_value(&gfx->frame_semaphore);
    gfx_execute_callbacks(completed_value);

    gfx->current_frame += 1;
}

void gfx_request_swapchain_resize(u32 width, u32 height) {
    gfx->resize_requested = true;
    gfx->resize_width     = width;
    gfx->resize_height    = height;
}

GPU_Material gpu_material_from_gfx(GFX_Material *material) {
    GPU_Material result = {};

    result.albedo    = material->albedo;
    result.metallic  = material->metallic;
    result.roughness = material->roughness;

    result.albedo_id = gfx_bindless_from_texture(material->albedo_texture, RHI_TEXTURE_VIEW_TYPE_SAMPLED);
    result.orm_id    = gfx_bindless_from_texture(material->orm_texture, RHI_TEXTURE_VIEW_TYPE_SAMPLED);

    return result;
}
