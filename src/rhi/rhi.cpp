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

    switch (kind) {
        case RHI_KIND_D3D12:
            return d3d12_list_init(&device->d3d12, &buffer->d3d12, type);

        default:
            Assert(0);
    }

    return false;
}

void rhi_command_buffer_deinit(RHI_Command_Buffer *buffer) {
    switch (buffer->kind) {
        case RHI_KIND_D3D12:
            d3d12_list_deinit(&buffer->d3d12);
            break;

        default:
            Assert(0);
    }
}

bool rhi_surface_init(RHI_Device *device, RHI_Surface *surface, RHI_Surface_Desc desc) {
    memset(surface, 0, sizeof(*surface));
    RHI_Kind kind = device->kind;
    surface->kind = kind;
    surface->desc = desc;

    if (!(desc.num_back_buffers > 0 && desc.num_back_buffers <= RHI_MAX_BACK_BUFFER)) {
        return false;
    }

    switch (kind) {
        case RHI_KIND_D3D12:
            return d3d12_surface_init(&device->d3d12, &surface->d3d12, desc);

        default:
            Assert(0);
    }

    return false;
}

void rhi_surface_present(RHI_Surface *surface) {
    switch (surface->kind) {
        case RHI_KIND_D3D12:
            d3d12_surface_present(&surface->d3d12);
            break;

        default:
            Assert(0);
            break;
    }
}


//
// Render Pass
//
void rhi_render_pass_begin(RHI_Command_Buffer *cmd_buffer, RHI_Render_Pass *render_pass) {
    switch (cmd_buffer->kind) {
        case RHI_KIND_D3D12:
            d3d12_render_pass_begin(cmd_buffer, render_pass);
            break;

        default:
            Assert(0);
            break;
    }
}

void rhi_render_pass_end(RHI_Command_Buffer *cmd_buffer, RHI_Render_Pass *render_pass) {
    switch (cmd_buffer->kind) {
        case RHI_KIND_D3D12:
            d3d12_render_pass_end(cmd_buffer, render_pass);
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
