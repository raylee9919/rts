// Copyright Seong Woo Lee. All Rights Reserved.

bool rhi_device_init(RHI_Device *device, RHI_Kind kind, bool debug, bool break_on_warning) {
    memset(device, 0, sizeof(*device));
    device->kind = kind;

    switch (kind) {
        case RHI_KIND_D3D12:
            return d3d12_device_init(device, debug, break_on_warning);

        default:
            Assert(0);
    }

    return false;
}

void rhi_device_deinit(RHI_Device *device) {
    switch (device->kind) {
        case RHI_KIND_D3D12:
            d3d12_device_deinit(device);
            break;

        default:
            Assert(0);
    }
}

bool rhi_command_buffer_init(RHI_Device *device, RHI_Command_Buffer *buffer, RHI_Command_Type type) {
    memset(buffer, 0, sizeof(*buffer));
    RHI_Kind kind = device->kind;
    buffer->kind = kind;
    buffer->type = type;

    switch (kind) {
        case RHI_KIND_D3D12:
            return d3d12_command_list_init(device, buffer, type);

        default:
            Assert(0);
    }

    return false;
}

void rhi_command_buffer_deinit(RHI_Command_Buffer *cmd_buffer) {
    switch (cmd_buffer->kind) {
        case RHI_KIND_D3D12:
            d3d12_command_list_deinit(cmd_buffer);
            break;

        default:
            Assert(0);
    }
}

void rhi_command_buffer_begin(RHI_Command_Buffer *cmd_buffer) {
    switch (cmd_buffer->kind) {
        case RHI_KIND_D3D12:
            d3d12_command_list_begin(cmd_buffer);
            break;

        default:
            Assert(0);
    }
}

void rhi_command_buffer_end(RHI_Command_Buffer *cmd_buffer) {
    switch (cmd_buffer->kind) {
        case RHI_KIND_D3D12:
            d3d12_command_list_end(&cmd_buffer->d3d12);
            break;

        default:
            Assert(0);
    }
}


//
// Submit
//
void rhi_submit(RHI_Device *device, u32 count, RHI_Command_Buffer **cmd_buffers) {
    switch (device->kind) {
        case RHI_KIND_D3D12:
            d3d12_submit(device, count, cmd_buffers);
            break;

        default:
            Assert(0);
    }
}


//
// Surface
//
bool rhi_surface_init(RHI_Device *device, RHI_Surface *surface, RHI_Surface_Desc *desc) {
    memset(surface, 0, sizeof(*surface));
    RHI_Kind kind = device->kind;
    surface->kind = kind;
    surface->desc = *desc;

    if (!(desc->num_back_buffers > 0 && desc->num_back_buffers <= RHI_MAX_BACK_BUFFERS)) {
        log(LOG_ERROR, S("Number of backbuffers must be less or equal than %d."), RHI_MAX_BACK_BUFFERS);
        return false;
    }

    switch (kind) {
        case RHI_KIND_D3D12:
            return d3d12_surface_init(device, surface, desc);

        default:
            Assert(0);
    }

    return false;
}

void rhi_surface_present(RHI_Surface *surface) {
    switch (surface->kind) {
        case RHI_KIND_D3D12:
            d3d12_surface_present(surface);
            break;

        default:
            Assert(0);
            break;
    }
}

void rhi_surface_resize(RHI_Surface *surface, u32 width, u32 height) {
    switch (surface->kind) {
        case RHI_KIND_D3D12:
            d3d12_surface_resize(surface, width, height);
            break;

        default:
            Assert(0);
            break;
    }
}


//
// Buffer
//
bool rhi_buffer_init(RHI_Device *device, RHI_Buffer *buffer, RHI_Buffer_Desc *desc, RHI_Heap *heap) {
    buffer->kind = device->kind;
    buffer->desc = *desc;

    switch (device->kind) {
        case RHI_KIND_D3D12:
            return d3d12_buffer_init(device, buffer, desc, heap);

        default:
            Assert(0);
            return false;
    }
}

void rhi_buffer_deinit(RHI_Buffer *buffer) {
    switch (buffer->kind) {
        case RHI_KIND_D3D12:
            d3d12_buffer_deinit(buffer);
            break;

        default:
            Assert(0);
            break;
    }
}

void *rhi_buffer_map(RHI_Buffer *buffer) {
    switch (buffer->kind) {
        case RHI_KIND_D3D12:
            return d3d12_buffer_map(buffer);

        default:
            Assert(0);
            return NULL;
    }
}

void rhi_buffer_unmap(RHI_Buffer *buffer) {
    switch (buffer->kind) {
        case RHI_KIND_D3D12:
            d3d12_buffer_unmap(buffer);
            break;

        default:
            Assert(0);
            break;
    }
}

void rhi_buffer_view_init(RHI_Device *device, RHI_Buffer_View *view, RHI_Buffer *buffer, RHI_Buffer_View_Desc *desc) {
    view->kind = device->kind;
    view->desc = *desc;

    switch (device->kind) {
        case RHI_KIND_D3D12:
            d3d12_buffer_view_init(device, view, buffer, desc);
            break;

        default:
            Assert(0);
            break;
    }
}

void rhi_buffer_view_deinit(RHI_Buffer_View *view) {
    switch (view->kind) {
        case RHI_KIND_D3D12:
            d3d12_buffer_view_deinit(view);
            break;

        default:
            Assert(0);
            break;
    }
}


//
// Texture
//
bool rhi_texture_create(RHI_Device *device, RHI_Texture *texture, RHI_Texture_Desc *desc, RHI_Heap *heap) {
    texture->kind = device->kind;

    switch (device->kind) {
        case RHI_KIND_D3D12:
            return d3d12_texture_create(device, texture, desc, heap);

        default:
            Assert(0);
            return false;
    }
}

void rhi_texture_destroy(RHI_Texture *texture) {
    switch (texture->kind) {
        case RHI_KIND_D3D12:
            d3d12_texture_destroy(texture);
            break;

        default:
            Assert(0);
            break;
    }
}

void rhi_texture_view_create(RHI_Device *device, RHI_Texture_View *view, RHI_Texture *texture, RHI_Texture_View_Desc *desc) {
    view->kind = device->kind;
    view->desc = *desc;

    switch (device->kind) {
        case RHI_KIND_D3D12:
            d3d12_texture_view_create(device, view, texture, desc);
            break;

        default:
            Assert(0);
            break;
    }
}

void rhi_texture_view_destroy(RHI_Texture_View *view) {
    switch (view->kind) {
        case RHI_KIND_D3D12:
            d3d12_texture_view_destroy(view);
            break;

        default:
            Assert(0);
            break;
    }
}


//
// Render Pass
//
void rhi_pass_begin(RHI_Command_Buffer *cmd_buffer, RHI_Pass *render_pass) {
    switch (cmd_buffer->kind) {
        case RHI_KIND_D3D12:
            d3d12_pass_begin(cmd_buffer, render_pass);
            break;

        default:
            Assert(0);
            break;
    }
}

void rhi_pass_end(RHI_Command_Buffer *cmd_buffer, RHI_Pass *render_pass) {
    switch (cmd_buffer->kind) {
        case RHI_KIND_D3D12:
            d3d12_pass_end(cmd_buffer, render_pass);
            break;

        default:
            Assert(0);
            break;
    }
}


//
// Fence
//
bool rhi_semaphore_create(RHI_Device *device, RHI_Semaphore *semaphore) {
    semaphore->kind = device->kind;

    switch (semaphore->kind) {
        case RHI_KIND_D3D12:
            return d3d12_fence_create(device, semaphore);

        default:
            Assert(0);
            return false;
    }
}

void rhi_semaphore_destroy(RHI_Semaphore *semaphore) {
    switch (semaphore->kind) {
        case RHI_KIND_D3D12:
            d3d12_fence_destroy(semaphore);
            break;

        default:
            Assert(0);
            break;
    }
}

void rhi_semaphore_wait(RHI_Semaphore *semaphore, u64 value, u32 timeout) {
    switch (semaphore->kind) {
        case RHI_KIND_D3D12:
            d3d12_fence_wait(semaphore, value, timeout);
            break;

        default:
            Assert(0);
            break;
    }
}

void rhi_semaphore_signal(RHI_Device *device, RHI_Command_Type queue_type, RHI_Semaphore *semaphore, u64 value) {
    switch (device->kind) {
        case RHI_KIND_D3D12:
            d3d12_queue_signal(device, queue_type, semaphore, value);
            break;

        default:
            Assert(0);
            break;
    }
}

void rhi_queue_wait(RHI_Device *device, RHI_Command_Type queue_type, RHI_Semaphore *semaphore, u64 value) {
    switch (device->kind) {
        case RHI_KIND_D3D12:
            d3d12_queue_wait(device, queue_type, semaphore, value);
            break;

        default:
            Assert(0);
            break;
    }
}


//
// Pipeline
//
bool rhi_pipeline_init(RHI_Device *device, RHI_Pipeline *pipeline, RHI_Pipeline_Desc *desc) {
    pipeline->kind = device->kind;

    switch (device->kind) {
        case RHI_KIND_D3D12:
            return d3d12_pipeline_init(device, pipeline, desc);

        default:
            Assert(0);
            return false;
    }
}

void rhi_pipeline_deinit(RHI_Pipeline *pipeline) {
    switch (pipeline->kind) {
        case RHI_KIND_D3D12:
            d3d12_pipeline_deinit(pipeline);
            break;

        default:
            Assert(0);
            break;
    }
}


//
// Commands
//
void rhi_cmd_texture_barrier(RHI_Command_Buffer *cmd_buffer, RHI_Texture *texture, RHI_Resource_State before, RHI_Resource_State after, u32 mip, u32 slice) {
    switch (cmd_buffer->kind) {
        case RHI_KIND_D3D12:
            d3d12_cmd_texture_barrier(cmd_buffer, texture, before, after, mip, slice);
            break;

        default:
            Assert(0);
            break;
    }
}

void rhi_cmd_set_pipeline(RHI_Command_Buffer *cmd_buffer, RHI_Pipeline *pipeline) {
    switch (cmd_buffer->kind) {
        case RHI_KIND_D3D12:
            d3d12_cmd_set_pipeline(cmd_buffer, pipeline);
            break;

        default:
            Assert(0);
            break;
    }
}

void rhi_cmd_set_viewport(RHI_Command_Buffer *cmd_buffer, float x, float y, float width, float height, float min_depth, float max_depth) {
    switch (cmd_buffer->kind) {
        case RHI_KIND_D3D12:
            d3d12_cmd_set_viewport(cmd_buffer, x, y, width, height, min_depth, max_depth);
            break;

        default:
            Assert(0);
            break;
    }
}

void rhi_cmd_set_scissor(RHI_Command_Buffer *cmd_buffer, u32 x, u32 y, u32 width, u32 height) {
    switch (cmd_buffer->kind) {
        case RHI_KIND_D3D12:
            d3d12_cmd_set_scissor(cmd_buffer, x, y, width, height);
            break;

        default:
            Assert(0);
            break;
    }
}

void rhi_cmd_draw(RHI_Command_Buffer *cmd_buffer, u32 num_vertices, u32 num_instances, u32 first_vertex, u32 first_instance) {
    switch (cmd_buffer->kind) {
        case RHI_KIND_D3D12:
            d3d12_cmd_draw(cmd_buffer, num_vertices, num_instances, first_vertex, first_instance);
            break;

        default:
            Assert(0);
            break;
    }
}

void rhi_cmd_draw_indexed(RHI_Command_Buffer *cmd_buffer, RHI_Buffer *index_buffer, u32 index_size, u32 num_indices, u32 num_instances, u32 first_index, u32 first_vertex, u32 first_instance) {
    switch (cmd_buffer->kind) {
        case RHI_KIND_D3D12:
            d3d12_cmd_draw_indexed(cmd_buffer, index_buffer, index_size, num_indices, num_instances, first_index, first_vertex, first_instance);
            break;

        default:
            Assert(0);
            break;
    }
}

void rhi_cmd_push_constants(RHI_Command_Buffer *cmd_buffer, void *data, u64 size) {
    switch (cmd_buffer->kind) {
        case RHI_KIND_D3D12:
            d3d12_cmd_push_constants(cmd_buffer, data, size);
            break;

        default:
            Assert(0);
            break;
    }
}
