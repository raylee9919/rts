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

void gfx_init(GFX_Init init)
{
    Arena *arena = arena_alloc();
    gfx = push_struct(arena, GFX_State);
    gfx->arena = arena;

    gfx->init = init;

    gfx->device = push_struct(arena, RHI_Device);
    Assert(rhi_device_init(gfx->device, init.kind, init.debug, init.break_on_warning));

    gfx->surface = push_struct(arena, RHI_Surface);
    gfx_init_surface(init.native_window_handle, init.width, init.height, init.num_back_buffers);

    gfx_init_samplers();

    gfx_init_uploader(128llu * 1024llu * 1024llu);

    // @Temporary: These will go to encoding threads.
    for (u32 i = 0; i < RHI_MAX_BACK_BUFFERS; ++i) 
    {
        Assert(rhi_command_buffer_init(gfx->device, &gfx->command_buffers[i], RHI_COMMAND_TYPE_GRAPHICS));
        Assert(rhi_command_buffer_init(gfx->device, &gfx->compute_buffers[i], RHI_COMMAND_TYPE_COMPUTE));
    }
}

void gfx_shutdown()
{
    gfx_deinit_samplers();
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

void gfx_register_mesh(u64 id, void *vertices, u32 num_vertices, u32 vertex_size, void *indices, u32 num_indices, u32 index_size)
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

    table_add(&gfx->mesh_table, id, entry);

    { // @Temporary: Upload vertex buffer and index buffer.
        void *ptr = NULL;

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

void gfx_unregister_mesh(u64 id)
{
    auto *mesh = table_find_pointer(&gfx->mesh_table, id);
    if (mesh)
    {
        rhi_buffer_view_deinit(&mesh->vertex_buffer_view);
        rhi_buffer_deinit(&mesh->vertex_buffer);
        rhi_buffer_deinit(&mesh->index_buffer);
    }
}

void gfx_draw(u64 mesh_id)
{
    auto *mesh = table_find_pointer(&gfx->mesh_table, mesh_id);

    if (!mesh && gfx->init.debug)
    {
        log(LOG_WARNING, S("Mesh wasn't registered."));
        debug_break();
    }

    if (mesh)
    {
    }
}

void gfx_end()
{
}
