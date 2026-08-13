// Copyright Seong Woo Lee. All Rights Reserved.

static void gfx_init_samplers()
{
    RHI_Sampler_Desc desc = {};
    {
        desc.filter             = RHI_FILTER_NEAREST;
        desc.address_u          = RHI_ADDRESS_REPEAT;
        desc.address_v          = RHI_ADDRESS_REPEAT;
        desc.address_w          = RHI_ADDRESS_REPEAT;
        desc.compare_op         = RHI_COMPARE_ALWAYS;
        desc.mip_lod_bias       = 0.f;
        desc.min_lod            = 0.f;
        desc.max_lod            = 1e10f;
    }
    rhi_sampler_init(gfx->device, &gfx->linear_sampler, &desc);
}

static void gfx_deinit_samplers()
{
    rhi_sampler_deinit(&gfx->linear_sampler);
}

static void gfx_init_surface(void *native_window_handle, u32 width, u32 height, u32 num_back_buffers)
{
    RHI_Surface_Desc desc = {};
    {
        desc.native_window_handle = native_window_handle;
        desc.width                = width;
        desc.height               = height;
        desc.num_back_buffers     = num_back_buffers;
    }
    Assert(rhi_surface_init(gfx->device, gfx->surface, &desc));
}

static void gfx_deinit_surface()
{
    // rhi_surface_deinit(gfx->surface);
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

static void gfx_reset_per_frame_data()
{
    // Reset per-frame data.
    gfx->current_pass           = UINT32_MAX;
    gfx->ctx_pipeline           = {};
    gfx->current_push_constants = 0;

    array_reset_keeping_memory(&gfx->sort_keys);
    array_reset_keeping_memory(&gfx->commands);
    array_reset_keeping_memory(&gfx->push_constants);

    memset(&gfx->pass_states[0], 0, sizeof(gfx->pass_states[0]) * GFX_MAX_PASS);
}

static force_inline u32 gfx_rand32()
{
    return xorshift32();
}

static GFX_Handle gfx_generate_handle()
{
    GFX_Handle handle;
    u64 hi = gfx->generational_handle_id++;
    u32 lo = gfx_rand32();
    handle.u[0] = (hi << 32llu) | lo;
    return handle;
}

static bool gfx_handle_is_null(GFX_Handle handle)
{
    return handle.u[0] == 0;
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

void gfx_init(GFX_Info info)
{
    // Alloc, ZII
    Arena *arena = arena_alloc();
    gfx = push_struct(arena, GFX_State);
    gfx->arena = arena;

    // Construct
    Construct(gfx);

    gfx->info = info;

    gfx->device = push_struct(arena, RHI_Device);
    Assert(rhi_device_init(gfx->device, info.kind, info.debug, info.break_on_warning));

    gfx->surface = push_struct(arena, RHI_Surface);
    gfx_init_surface(info.native_window_handle, info.width, info.height, info.num_back_buffers);

    for (u32 i = 0; i < RHI_MAX_BACK_BUFFERS; ++i) {
        RHI_Texture_View_Desc desc = {};
        {
            desc.type              = RHI_TEXTURE_VIEW_TYPE_RENDER_TARGET;
            desc.dimension         = RHI_TEXTURE_TYPE_2D;
            desc.format            = gfx->surface->textures[i].desc.format;
            desc.base_mip_level    = 0;
        }
        rhi_texture_view_init(gfx->device, &gfx->surface_views[i], &gfx->surface->textures[i], &desc);
    }

    rhi_semaphore_init(gfx->device, &gfx->semaphore);

    gfx_init_samplers();

    gfx_init_uploader(MB(128)); // @Temporary

    // @Temporary: These will go to encoding threads.
    for (u32 i = 0; i < RHI_MAX_BACK_BUFFERS; ++i) 
    {
        Assert(rhi_command_buffer_init(gfx->device, &gfx->command_buffers[i], RHI_COMMAND_TYPE_GRAPHICS));
        Assert(rhi_command_buffer_init(gfx->device, &gfx->compute_buffers[i], RHI_COMMAND_TYPE_COMPUTE));
    }

    GFX_SURFACE_TEXTURE.handle = gfx_generate_handle();
}

void gfx_shutdown()
{
    // @Temporary: These will go to encoding threads.
    for (u32 i = 0; i < RHI_MAX_BACK_BUFFERS; ++i) 
    {
        rhi_command_buffer_deinit(&gfx->command_buffers[i]);
        rhi_command_buffer_deinit(&gfx->compute_buffers[i]);
    }

    // @Todo: release resources safely..

    gfx_deinit_samplers();
    rhi_semaphore_deinit(&gfx->semaphore);
    gfx_deinit_surface();
    rhi_device_deinit(gfx->device);

    arena_release(gfx->arena);
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

static GFX_Sort_Key gfx_encode_sort_key(u64 pass, u64 pipeline, u64 push_constants)
{
    // @Temporary
    GFX_Sort_Key key = {};
    key.bits = (pass << 16) | (pipeline << 8) | push_constants;

    return key;
}

void gfx_mesh_create(u64 id, void *vertices, u32 num_vertices, u32 vertex_size, void *indices, u32 num_indices, u32 index_size)
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

    table_add(&gfx->mesh_table, id, entry);

    { // @Temporary: Upload vertex buffer and index buffer.
        void *ptr = 0;

        u64 vb_sz = num_vertices * vertex_size;
        ptr = rhi_buffer_map(&gfx->upload_buffer);
        memcpy(ptr, vertices, vb_sz);

        u64 ib_sz = num_indices * index_size;
        ptr = rhi_buffer_map(&gfx->upload_buffer);
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

void gfx_mesh_destroy(u64 id)
{
    // Destruction is deferred until the current workload on the GPU is done.
    GFX_Callback_Entry entry = {};
    entry.semaphore_value_to_execute = gfx->current_frame;
    entry.proc    = gfx_mesh_destroy_callback;
    entry.mesh_id = id;

    gfx_add_callback(entry);
}

GFX_Texture gfx_texture_create(RHI_Texture_Desc desc)
{
    GFX_Texture result = {};
    GFX_Texture_Entry entry = {};

    Assert( rhi_texture_init(gfx->device, &entry.texture, &desc, NULL) );

    RHI_Texture_View_Type view_type = 0;

    for (u32 flag = 0x1; flag < RHI_TEXTURE_USAGE_OPL_BIT; flag <<= 1) 
    {
        if (desc.usage & flag) 
        {
            RHI_Texture_View_Desc vdesc = {};
            vdesc.type               = view_type;
            vdesc.dimension          = desc.type;
            vdesc.format             = desc.format;
            vdesc.base_mip_level     = 0;
            vdesc.base_array_layer   = 0;
            vdesc.mip_levels         = desc.mip_levels;
            vdesc.depth              = desc.depth;

            rhi_texture_view_init(gfx->device, &entry.views[view_type], &entry.texture, &vdesc);

            result.bindless[view_type] = entry.views[view_type].bindless;
        }
        else
        {
            result.bindless[view_type] = UINT32_MAX;
        }

        view_type += 1;
    }

    result.handle = gfx_generate_handle();

    table_add(&gfx->texture_table, result.handle, entry);

    return result;
}

void gfx_texture_destroy(GFX_Texture texture)
{
    auto *entry = table_find_pointer(&gfx->texture_table, texture.handle);
    if (entry)
    {
        for (u16 i = 0; i < RHI_TEXTURE_VIEW_TYPE_COUNT; ++i) 
        {
            if (entry->views[i].kind != RHI_KIND_INVALID)
            {
                rhi_texture_view_deinit(&entry->views[i]);
            }
        }
        rhi_texture_deinit(&entry->texture);
        
        table_remove(&gfx->texture_table, texture.handle);
    }
}

void gfx_texture_upload(GFX_Texture texture, RHI_Texture_Format format, void *data, u32 size, u32 width, u32 height)
{
    auto *tex = table_find_pointer(&gfx->texture_table, texture.handle);

    if (tex)
    {
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
        {
            box.width  = width;
            box.height = height;
            box.depth  = 1;
        }

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

void gfx_pass_begin(u32 pass_index)
{
    Assert(pass_index < GFX_MAX_PASS);
    gfx->current_pass = pass_index;
}

void gfx_pass_end()
{
    gfx->current_pass = UINT32_MAX;
}

void gfx_pass_color_attachment(u32 pass_index, u32 color_attachment_index, GFX_Texture texture)
{
    Assert(pass_index < GFX_MAX_PASS);

    auto *pass = &gfx->pass_states[pass_index];

    pass->color_attachments[color_attachment_index] = texture;
}

void gfx_pass_depth_attachment(u32 pass_index, GFX_Texture texture)
{
    Assert(pass_index < GFX_MAX_PASS);

    auto *pass = &gfx->pass_states[pass_index];

    pass->depth_attachment = texture;
}

void gfx_pass_viewport(u32 pass_index, f32 top_left_x, f32 top_left_y, f32 width, f32 height)
{
    Assert(pass_index < GFX_MAX_PASS);

    auto *vp = &gfx->pass_states[pass_index].viewport;

    if (vp->set && gfx->info.debug)
        log(LOG_WARNING, S("Viewport of pass %d will be overwritten."), pass_index);

    vp->set = true;
    vp->x = top_left_x;
    vp->y = top_left_y;
    vp->w = width;
    vp->h = height;
}

void gfx_pass_scissor(u32 pass_index, u32 top_left_x, u32 top_left_y, u32 width, u32 height)
{
    Assert(pass_index < GFX_MAX_PASS);

    auto *sc = &gfx->pass_states[pass_index].scissor;

    if (sc->set && gfx->info.debug)
        log(LOG_WARNING, S("Scissor of pass %d will be overwritten."), pass_index);

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

GFX_Pipeline gfx_pipeline_create(RHI_Pipeline_Desc desc)
{
    Assert(gfx->generational_pipeline_id < GFX_MAX_PIPELINE);

    GFX_Pipeline result = {};

    GFX_Pipeline_Entry entry = {};
    Assert( rhi_pipeline_init(gfx->device, &entry.rhi_pipeline, &desc) );

    result.handle = gfx_generate_handle();
    result.index  = gfx->generational_pipeline_id++;

    table_add(&gfx->pipeline_table, result.handle, entry);

    return result;
}

void gfx_pipeline_destroy(GFX_Pipeline pipeline)
{
    auto *entry = table_find_pointer(&gfx->pipeline_table, pipeline.handle);
    if (entry)
    {
        rhi_pipeline_deinit(&entry->rhi_pipeline);
        table_remove(&gfx->pipeline_table, pipeline.handle);
    }
}

void gfx_pipeline(GFX_Pipeline pipeline)
{
    gfx->ctx_pipeline = pipeline;
}

void gfx_push_constants(void *data, u32 size)
{
    Assert(size <= (4 * RHI_MAX_32BIT_PUSH_CONSTANTS));

    array_add(&gfx->push_constants, {});
    auto *push = &gfx->push_constants;
    auto *entry = &push->data[push->count - 1];
    void *dst = &entry->data[0];
    memcpy(dst, data, size);

    entry->size = size;

    gfx->current_push_constants += 1;
}

void gfx_draw(u64 mesh_id, u32 num_instances)
{
    Assert(gfx->current_pass != UINT32_MAX);

    auto *mesh = table_find_pointer(&gfx->mesh_table, mesh_id);

    if (mesh)
    {
        // @Robustness
        GFX_Sort_Key key = gfx_encode_sort_key(gfx->current_pass,
                                               gfx->ctx_pipeline.index,
                                               gfx->current_push_constants);  
        key.cmd_index = gfx->commands.count;
        array_add(&gfx->sort_keys, key);

        GFX_Command cmd = {};
        cmd.pipeline = gfx->ctx_pipeline;
        cmd.push_constant_index = gfx->current_push_constants - 1; // can wrap to UINT32_MAX if nothing was pushed.
        cmd.mesh_handle = mesh_id;
        cmd.num_instances = num_instances;

        array_add(&gfx->commands, cmd);
    }
    else if (gfx->info.debug)
    {
        log(LOG_WARNING, S("Mesh wasn't registered."));
        Assert(0);
    }
}

void gfx_end()
{
    Assert(gfx->push_constants.count == gfx->current_push_constants);

    auto *cmd_buffer = &gfx->command_buffers[gfx->current_frame % gfx->info.num_back_buffers];

    // @Temporary
    rhi_command_buffer_begin(cmd_buffer);
    {
        for (u32 pass_idx = 0; pass_idx < GFX_MAX_PASS; ++pass_idx)
        {
            // @Temporary, @Todo: Barriers should be set automatically...!
            rhi_cmd_texture_barrier(cmd_buffer, &gfx->surface->textures[gfx->surface->current_frame_index], RHI_RESOURCE_STATE_COMMON, RHI_RESOURCE_STATE_RENDER_TARGET, RHI_ALL_MIPS, RHI_ALL_LAYERS);

            auto *p = &gfx->pass_states[pass_idx];
            auto vp = p->viewport;
            auto sc = p->scissor;

            RHI_Pass pass = {};

            for (u32 i = 0; i < RHI_MAX_COLOR_ATTACHMENTS; ++i)
            {
                if (!gfx_handle_is_null(p->color_attachments[i].handle))
                {
                    pass.num_color_attachments += 1;
                }
            }


            if (!gfx_handle_is_null(p->depth_attachment.handle))
            {
                pass.has_depth_attachment = true;
            }

            for (u16 color_idx = 0; color_idx < RHI_MAX_COLOR_ATTACHMENTS; ++color_idx) 
            {
                if (p->flags & (GFX_PASS_FLAG_CLEAR_COLOR_0 << (color_idx)))
                {
                    GFX_Handle handle = p->color_attachments[color_idx].handle;

                    // @Cleanup: I don't like this additional code path.
                    if (handle != GFX_SURFACE_TEXTURE.handle)
                    {
                        auto *tex = table_find_pointer(&gfx->texture_table, handle);
                        if (tex)
                        {
                            auto *attachment = &pass.color_attachments[color_idx];
                            attachment->view = tex->views[RHI_TEXTURE_VIEW_TYPE_RENDER_TARGET];

                            attachment->load_op = RHI_LOAD_OP_CLEAR;

                            u32 c_packed = p->colors[color_idx];
                            v4 c_unpacked = unpack_rgba(c_packed);
                            memcpy(attachment->clear_color, &c_unpacked, sizeof(f32) * 4);
                        }
                        else
                        {
                            // @Todo: Error-handling.
                            log(LOG_ERROR, S("Color attachment texture wasn't found."));
                            Assert(0);
                        }
                    }
                    else
                    {
                        auto *attachment = &pass.color_attachments[color_idx];
                        attachment->view = gfx->surface_views[gfx->surface->current_frame_index];

                        attachment->load_op = RHI_LOAD_OP_CLEAR;

                        u32 c_packed = p->colors[color_idx];
                        v4 c_unpacked = unpack_rgba(c_packed);
                        memcpy(attachment->clear_color, &c_unpacked, sizeof(f32) * 4);
                    }
                }
            }

            if (p->flags & GFX_PASS_FLAG_CLEAR_DEPTH_STENCIL)
            {
                GFX_Handle handle = p->depth_attachment.handle;
                auto *tex = table_find_pointer(&gfx->texture_table, handle);

                if (tex)
                {
                    auto *attachment = &pass.depth_attachment;
                    attachment->view        = tex->views[RHI_TEXTURE_VIEW_TYPE_DEPTH_STENCIL];;
                    attachment->load_op     = RHI_LOAD_OP_CLEAR;
                    attachment->clear_depth = p->depth;
                }
                else
                {
                    // @Todo: Error-handling.
                    log(LOG_ERROR, S("Depth attachment texture wasn't found."));
                    Assert(0);
                }
            }

            rhi_pass_begin(cmd_buffer, &pass);
            {
                rhi_cmd_set_viewport(cmd_buffer, vp.x, vp.y, vp.w, vp.h, 0.f, 1.f);
                rhi_cmd_set_scissor(cmd_buffer, sc.x, sc.y, sc.w, sc.h);


                {
                    // 1. Sort commands (including resource managing calls).
                    // 2. ..

                    u64 current_pass     = UINT32_MAX;
                    u64 current_pipeline = UINT32_MAX; // @Fix: If pass changes, pipeline must be reset..

                    for (auto& key : gfx->sort_keys)
                    {
                        // @Temporary: Decode the key.
                        u64 pass_               = (key.bits & 0b111111110000000000000000) >> 16;
                        u64 pipeline            = (key.bits & 0b1111111100000000) >> 8;
                        u64 push_constant_index = (key.bits & 0b11111111);

                        // Fetch the corresponding command.
                        auto cmd = gfx->commands[key.cmd_index];

                        // Update pass? @Todo
                        //if (current_pass != pass)
                        //{
                        //    current_pass = pass;
                        //}

                        // Update pipeline?
                        if (current_pipeline != cmd.pipeline.index)
                        {
                            current_pipeline = cmd.pipeline.index;
                            auto *entry = table_find_pointer(&gfx->pipeline_table, cmd.pipeline.handle);
                            Assert(entry);
                            rhi_cmd_set_pipeline(cmd_buffer, &entry->rhi_pipeline);
                        }

                        // And the real push constants.
                        if (cmd.push_constant_index != UINT32_MAX)
                        {
                            auto* entry = &gfx->push_constants.data[cmd.push_constant_index];
                            rhi_cmd_push_constants(cmd_buffer, entry->data, entry->size);
                        }

                        auto *mesh = table_find_pointer(&gfx->mesh_table, cmd.mesh_handle);

                        rhi_cmd_draw_indexed(cmd_buffer, &mesh->index_buffer, mesh->index_size, mesh->num_indices, cmd.num_instances, 0, 0, 0);
                    }
                }


            }
            rhi_pass_end(cmd_buffer, &pass);

            // @Temporary, @Todo: Barriers should be set automatically...!
            rhi_cmd_texture_barrier(cmd_buffer, &gfx->surface->textures[gfx->surface->current_frame_index], RHI_RESOURCE_STATE_RENDER_TARGET, RHI_RESOURCE_STATE_PRESENT, RHI_ALL_MIPS, RHI_ALL_LAYERS);
        }
    }
    rhi_command_buffer_end(cmd_buffer);

    RHI_Command_Buffer *buffers[] = { cmd_buffer };
    rhi_submit(gfx->device, 1, buffers);

    rhi_semaphore_signal(gfx->device, RHI_COMMAND_TYPE_GRAPHICS, &gfx->semaphore, gfx->current_frame);

    rhi_surface_present(gfx->surface);

    gfx_reset_per_frame_data();

    u64 completed_value = rhi_semaphore_completed_value(&gfx->semaphore);
    gfx_execute_callbacks(completed_value);

    gfx->current_frame += 1;

    if (gfx->current_frame > gfx->info.num_back_buffers)
    {
        rhi_semaphore_wait(&gfx->semaphore, gfx->current_frame % gfx->info.num_back_buffers + 1, -1);
    }
}
