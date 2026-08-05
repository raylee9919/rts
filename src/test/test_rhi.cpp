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

struct Vertex {
    v3 position;
    v4 color;
};

Vertex vertices[3] = {
    { v3(-0.5f, -0.5f, 0.0f), v4(1.0f, 0.0f, 0.0f, 1.0f) },
    { v3( 0.5f, -0.5f, 0.0f), v4(0.0f, 1.0f, 0.0f, 1.0f) },
    { v3( 0.0f,  0.5f, 0.0f), v4(0.0f, 0.0f, 1.0f, 1.0f) }
};

u32 indices[3] = {
    0, 1, 2
};

struct Constants {
    u32 vertex_buffer_id;
};

String shader_source = S(R"(
    // Copyright Seong Woo Lee. All Rights Reserved.

    struct Push_Constants {
        uint vertex_buffer_id;
    };
    ConstantBuffer<Push_Constants> push : register(b0);

    struct Vertex {
        float3 position;
        float  padding;
        float4 color;
    };
    
    struct VS_Output {
        float4 sv_position : SV_POSITION;
        float3 position    : POSITION;
        float4 color       : COLOR;
    };
    
    VS_Output main_vs(uint vertex_id : SV_VertexID) {
        VS_Output result;

        StructuredBuffer<Vertex> vertex_buffer = ResourceDescriptorHeap[push.vertex_buffer_id];
        Vertex vert = vertex_buffer[vertex_id];

        result.sv_position = float4(vert.position, 1.0);
        result.position    = vert.position;
        result.color       = vert.color;

        return result;
    }
    
    float4 main_ps(VS_Output input) : SV_TARGET {
        return input.color;
    }
)");


int main_entry(int argc, char **argv)
{
    window = os_window_create(1920, 1080, utf8lit("rhi"));
    HWND hwnd = hwnd_from_os_handle(window);

    auto *compiler = (Shader_Compiler *)alloc(sizeof(Shader_Compiler));
    Assert(shader_compiler_init(compiler));

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


    RHI_Pipeline pipeline = {};
    Shader_Compile_Result vs = {};
    Shader_Compile_Result ps = {};
    { // Create PSO
        // @Todo: Should I compile twice..?
        {
            Shader_Compile_Options vs_opts = {};
            {
                // @Temporary
                vs_opts.stage  = SHADER_STAGE_VS;
                vs_opts.entry  = S("main_vs");
                vs_opts.source = shader_source;
                vs_opts.debug  = true;
            }
            Assert(shader_compile(compiler, vs_opts, &vs, tctx.allocator));


            Shader_Compile_Options ps_opts = {};
            {
                // @Temporary
                ps_opts.stage  = SHADER_STAGE_PS;
                ps_opts.entry  = S("main_ps");
                ps_opts.source = shader_source;
                ps_opts.debug  = true;
            }
            Assert(shader_compile(compiler, ps_opts, &ps, tctx.allocator));
        }

        {
            RHI_Pipeline_Desc desc = {};
            desc.type = RHI_PIPELINE_TYPE_GRAPHICS;

            // @Temporary
            desc.num_color_attachments       = 1;
            desc.color_attachment_formats[0] = RHI_TEXTURE_FORMAT_RGBA8_UNORM;

            desc.fill_mode      = RHI_FILL_SOLID;
            desc.cull_mode      = RHI_CULL_NONE;
            desc.depth_clip     = true;

            desc.topology       = RHI_TOPOLOGY_TRIANGLES;

            desc.vs_data        = vs.data;
            desc.vs_size        = vs.size;

            desc.ps_data        = ps.data;
            desc.ps_size        = ps.size;

            rhi_pipeline_init(device, &pipeline, &desc);
        }
    }


    RHI_Buffer vertex_buffer           = {};
    RHI_Buffer_View vertex_buffer_view = {};
    { // Create vertex buffer and view.
        u64 sz = align_up(sizeof(vertices), 16);

        RHI_Buffer_Desc desc = {};
        desc.memory_type = RHI_MEMORY_TYPE_CPU_TO_GPU;
        desc.size        = sz;

        Assert(rhi_buffer_init(device, &vertex_buffer, &desc, NULL));

        // @Temporary
        void *ptr = rhi_buffer_map(&vertex_buffer);
        memcpy(ptr, vertices, sz);
        rhi_buffer_unmap(&vertex_buffer);

        RHI_Buffer_View_Desc view_desc = {};
        {
            view_desc.type     = RHI_BUFFER_VIEW_TYPE_STRUCTURED;
            view_desc.writable = false;
            view_desc.stride   = sizeof(Vertex);
            view_desc.offset   = 0;
            view_desc.size     = vertex_buffer.desc.size;
        }

        rhi_buffer_view_init(device, &vertex_buffer_view, &vertex_buffer, &view_desc);
    }


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
                float color[4] = { 0.3f, 0.2f, 0.2f, 1.f };
                memcpy(pass.color_attachments[0].clear_color, color, sizeof(color));
            }

            rhi_cmd_texture_barrier(cmd_buffer, &surface->textures[surface->current_frame_index], RHI_RESOURCE_STATE_COMMON, RHI_RESOURCE_STATE_RENDER_TARGET, RHI_ALL_MIP_LEVELS, RHI_ALL_ARRAY_SLICES);

            rhi_pass_begin(cmd_buffer, &pass);
            {
                rhi_cmd_set_pipeline(cmd_buffer, &pipeline);
                rhi_cmd_set_viewport(cmd_buffer, 0.f, 0.f, 1920.f, 1080.f, 0.f, 1.f);
                rhi_cmd_set_scissor(cmd_buffer, 0, 0, 1920, 1080);

                Constants constants = {};
                {
                    constants.vertex_buffer_id = vertex_buffer_view.bindless;
                }
            
                rhi_cmd_push_constants(cmd_buffer, &constants, sizeof(constants));

                // @Todo: Bind index buffer
                rhi_cmd_draw(cmd_buffer, 3, 1, 0, 0);
            }
            rhi_pass_end(cmd_buffer, &pass);

            rhi_cmd_texture_barrier(cmd_buffer, &surface->textures[surface->current_frame_index], RHI_RESOURCE_STATE_RENDER_TARGET, RHI_RESOURCE_STATE_PRESENT, RHI_ALL_MIP_LEVELS, RHI_ALL_ARRAY_SLICES);

            rhi_command_buffer_end(cmd_buffer);

            rhi_submit(device, 1, &cmd_buffer);

            // @Temporary
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
