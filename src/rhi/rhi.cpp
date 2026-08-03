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
            return d3d12_command_list_init(&device->d3d12, &buffer->d3d12, type);

        default:
            Assert(0);
    }

    return false;
}

void rhi_command_buffer_deinit(RHI_Command_Buffer *buffer) {
    switch (buffer->kind) {
        case RHI_KIND_D3D12:
            d3d12_command_list_deinit(&buffer->d3d12);
            break;

        default:
            Assert(0);
    }
}

void rhi_command_buffer_begin(RHI_Command_Buffer *cmd_buffer) {
    switch (cmd_buffer->kind) {
        case RHI_KIND_D3D12:
            d3d12_command_list_begin(&cmd_buffer->d3d12);
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
