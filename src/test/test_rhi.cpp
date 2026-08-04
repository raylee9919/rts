// Copyright Seong Woo Lee. All Rights Reserved.

#include "basic/include.h"
#include "math/include.h"
#include "os/include.h"
#include "rhi/include.h"
#include "shader/include.h"

#include "basic/include.cpp"
#include "math/include.cpp"
#include "os/include.cpp"
#include "rhi/include.cpp"
#include "shader/include.cpp"

OS_Handle window;
bool should_close       = false;
u32 surface_width       = 1920;
u32 surface_height      = 1080;
u32 num_back_buffers    = 3;
u64 current_frame_index = 0;


String shader_source = S(R"(
    // Copyright Seong Woo Lee. All Rights Reserved.

    struct Push_Constants {
        uint vertex_buffer_id;
    };
    ConstantBuffer<Push_Constants> push : register(b0);

    struct Vertex {
        float3 position;
        float4 color;
    };
    
    struct PS_Input {
        float4 sv_position : SV_POSITION;
        float3 position    : POSITION;
        float4 color       : COLOR;
    };
    
    PS_Input main_vs(uint vertex_id : SV_VertexID) {
        PS_Input result;

        StructuredBuffer<Vertex> vertex_buffer = ResourceDescriptorHeap[push.vertex_buffer_id];
        Vertex vert = vertex_buffer[vertex_id];

        result.sv_position = float4(vert.position, 1.0);
        result.position    = vert.position;
        result.color       = vert.color;

        return result;
    }
    
    float4 main_ps(PS_Input input) {
        return input.color;
    }
)");


int main_entry(int argc, char **argv)
{
    window = os_window_create(1920, 1080, utf8lit("rhi"));
    HWND hwnd = hwnd_from_os_handle(window);

    auto *device = (RHI_Device *)alloc(sizeof(RHI_Device));
    Assert(rhi_device_init(device, RHI_KIND_D3D12, true, true));

    RHI_Semaphore semaphore = {};
    Assert(rhi_semaphore_create(device, &semaphore));

    auto *surface = (RHI_Surface *)alloc(sizeof(RHI_Surface));
    {
        RHI_Surface_Desc surface_desc = {};
        {
            surface_desc.native_window_handle = (void *)hwnd;
            surface_desc.width                = surface_width;
            surface_desc.height               = surface_height;
            surface_desc.num_back_buffers     = num_back_buffers;
        }
        Assert(rhi_surface_init(device, surface, &surface_desc));
    }

    RHI_Texture_View views[RHI_MAX_BACK_BUFFERS] = {};
    {
        for (u32 i = 0; i < RHI_MAX_BACK_BUFFERS; ++i) {
            RHI_Texture_View_Desc view_desc = {};
            {
                view_desc.type              = RHI_TEXTURE_VIEW_TYPE_RENDER_TARGET;
                view_desc.dimension         = RHI_TEXTURE_TYPE_2D;
                view_desc.format            = surface->textures[i].desc.format;
                view_desc.base_mip_level    = 0;
            }
            rhi_texture_view_create(device, &views[i], &surface->textures[i], &view_desc);
        }
    }


    auto *cmd_buffer = (RHI_Command_Buffer *)alloc(sizeof(RHI_Command_Buffer));
    Assert(rhi_command_buffer_init(device, cmd_buffer, RHI_COMMAND_TYPE_GRAPHICS));


    while (!should_close) {
        // Clear and poll events.
        os_clear_events();
        os_poll_events();

        // Close app if needed
        list_for(os->first_event, event) {
            // Alt + F4?
            bool alt_f4_pressed         = event->kind == OS_EVENT_PRESS && event->key == KEY_F4 && (event->modifiers & OS_MODIFIER_ALT);
            bool window_close_triggered = event->kind == OS_EVENT_WINDOW_CLOSE && event->window == window;


            if (alt_f4_pressed | window_close_triggered) {
                should_close = true;
                os_remove_event(event);
            }

            // Fullscreen
            bool alt_enter_pressed = event->kind == OS_EVENT_PRESS && event->key == KEY_RETURN && (event->modifiers & OS_MODIFIER_ALT);
            if (alt_enter_pressed) {
                os_window_toggle_fullscreen(window);
            }
        }


        // Render
        {
            rhi_command_buffer_begin(cmd_buffer);

            RHI_Pass pass = {}; // transient
            {
                pass.num_color_attachments = 1;
                pass.color_attachments[0].view    = views[surface->current_frame_index];
                pass.color_attachments[0].load_op = RHI_LOAD_OP_CLEAR;
                float color[4] = { 1.f, 0.f, 1.f, 1.f };
                memcpy(pass.color_attachments[0].clear_color, color, sizeof(color));
            }

            rhi_cmd_texture_barrier(cmd_buffer, &surface->textures[surface->current_frame_index], RHI_RESOURCE_STATE_COMMON, RHI_RESOURCE_STATE_RENDER_TARGET, RHI_ALL_MIP_LEVELS, RHI_ALL_ARRAY_SLICES);

            rhi_pass_begin(cmd_buffer, &pass);
            {

            }
            rhi_pass_end(cmd_buffer, &pass);

            rhi_cmd_texture_barrier(cmd_buffer, &surface->textures[surface->current_frame_index], RHI_RESOURCE_STATE_RENDER_TARGET, RHI_RESOURCE_STATE_PRESENT, RHI_ALL_MIP_LEVELS, RHI_ALL_ARRAY_SLICES);

            rhi_command_buffer_end(cmd_buffer);

            rhi_submit(device, 1, &cmd_buffer);

            rhi_semaphore_signal(device, RHI_COMMAND_TYPE_GRAPHICS, &semaphore, current_frame_index);
            rhi_semaphore_wait(&semaphore, current_frame_index, RHI_INFINITE);
            current_frame_index += 1;

            rhi_surface_present(surface);
        }

        clear_temporary_storage();
    }

    rhi_command_buffer_deinit(cmd_buffer);
    rhi_device_deinit(device);

    return 0;
}
