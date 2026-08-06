// Copyright Seong Woo Lee. All Rights Reserved.

#include "basic/include.h"
#include "math/include.h"
#include "os/include.h"
#include "asset/include.h"
#include "rhi/include.h"
#include "shader/include.h"

#include "basic/include.cpp"
#include "math/include.cpp"
#include "os/include.cpp"
#include "asset/include.cpp"
#include "rhi/include.cpp"
#include "shader/include.cpp"

OS_Handle window        = {};
b32 should_close        = false;
u32 SURFACE_WIDTH       = 1920;
u32 SURFACE_HEIGHT      = 1080;
u32 num_back_buffers    = 3;
u64 current_frame_index = 0;

String shader_source = S(R"(
    // Copyright Seong Woo Lee. All Rights Reserved.

    struct Push_Constants {
        uint vertex_buffer_id;
        uint texture_id;
        uint linear_sampler_id;
    };
    ConstantBuffer<Push_Constants> push : register(b0);

    struct Vertex {
        float3 position;
        float2 uv;
        float3 padding;
        float4 color;
    };
    
    struct VS_Output {
        float4 sv_position : SV_POSITION;
        float3 position    : POSITION;
        float2 uv          : UV;
        float4 color       : COLOR;
    };
    
    VS_Output main_vs(uint vertex_id : SV_VertexID) {
        VS_Output result;

        StructuredBuffer<Vertex> vertex_buffer = ResourceDescriptorHeap[push.vertex_buffer_id];
        Vertex vert = vertex_buffer[vertex_id];

        result.sv_position = float4(vert.position, 1.0);
        result.position    = vert.position;
        result.uv          = vert.uv;
        result.color       = vert.color;

        return result;
    }
    
    float4 main_ps(VS_Output input) : SV_TARGET {
        Texture2D <float4> color_texture  = ResourceDescriptorHeap[push.texture_id];
        SamplerState linear_sampler = SamplerDescriptorHeap[push.linear_sampler_id];
        float4 rgba = color_texture.Sample(linear_sampler, input.uv);

        return rgba;
    }
)");


struct Vertex {
    v3 position;
    v2 uv;
    v4 color;
};

Vertex vertices[] = {
    { v3(-0.5f, -0.5f, 0.0f), v2(0.f, 1.f), v4(1.0f, 0.0f, 0.0f, 1.0f) },
    { v3( 0.5f, -0.5f, 0.0f), v2(1.f, 1.f), v4(0.0f, 1.0f, 0.0f, 1.0f) },
    { v3(-0.5f,  0.5f, 0.0f), v2(0.f, 0.f), v4(0.0f, 0.0f, 1.0f, 1.0f) },
    { v3( 0.5f,  0.5f, 0.0f), v2(1.f, 0.f), v4(1.0f, 1.0f, 1.0f, 1.0f) },
};

u32 indices[] = {
    0, 1, 2,
    2, 1, 3
};
u32 num_indices = array_count(indices);

struct Constants {
    u32 vertex_buffer_id;
    u32 texture_id;
    u32 linear_sampler_id;
};

int main_entry(int argc, char **argv)
{
    log(LOG_TRACE,   S("Testing log, level: trace"));
    log(LOG_DEBUG,   S("Testing log, level: debug"));
    log(LOG_INFO,    S("Testing log, level: info"));
    log(LOG_WARNING, S("Testing log, level: warning"));
    log(LOG_ERROR,   S("Testing log, level: error"));
    log(LOG_FATAL,   S("Testing log, level: fatal"));

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
            surface_desc.width                = SURFACE_WIDTH;
            surface_desc.height               = SURFACE_HEIGHT;
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


    auto *cmd_buffer = alloc_t(RHI_Command_Buffer);
    Assert(rhi_command_buffer_init(device, cmd_buffer, RHI_COMMAND_TYPE_GRAPHICS));

    auto *compute_buffer = alloc_t(RHI_Command_Buffer);
    Assert(rhi_command_buffer_init(device, compute_buffer, RHI_COMMAND_TYPE_COMPUTE));

    auto *copy_buffer = alloc_t(RHI_Command_Buffer);
    Assert(rhi_command_buffer_init(device, copy_buffer, RHI_COMMAND_TYPE_TRANSFER));


    RHI_Sampler linear_sampler = {};
    {
        RHI_Sampler_Desc desc= {};
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
        rhi_sampler_init(device, &linear_sampler, &desc);
    }


    RHI_Pipeline pipeline = {};
    Shader_Compile_Result vs = {};
    Shader_Compile_Result ps = {};
    { // Compile shader
        Shader_Compile_Options vs_opts = {};
        {
            // @Temporary
            vs_opts.stage  = SHADER_STAGE_VS;
            vs_opts.entry  = S("main_vs");
            vs_opts.source = shader_source;
        }
        Assert(shader_compile(compiler, vs_opts, true, &vs, tctx.allocator));


        Shader_Compile_Options ps_opts = {};
        {
            // @Temporary
            ps_opts.stage  = SHADER_STAGE_PS;
            ps_opts.entry  = S("main_ps");
            ps_opts.source = shader_source;
        }
        Assert(shader_compile(compiler, ps_opts, true, &ps, tctx.allocator));

        // ..or you read bytes from your asset file
    }

    { // Create PSO
        RHI_Pipeline_Desc desc = {};
        desc.type = RHI_PIPELINE_TYPE_GRAPHICS;

        // @Temporary
        desc.num_color_attachments       = 1;
        desc.color_attachment_formats[0] = RHI_TEXTURE_FORMAT_RGBA8_UNORM;

        desc.fill_mode = RHI_FILL_SOLID;
        desc.cull_mode = RHI_CULL_CW;

        desc.topology = RHI_TOPOLOGY_TRIANGLES;

        desc.vs_data = vs.data;
        desc.vs_size = vs.size;

        desc.ps_data = ps.data;
        desc.ps_size = ps.size;

        rhi_pipeline_init(device, &pipeline, &desc);
    }


    RHI_Buffer upload_buffer = {};
    { // Create upload buffer
        u64 sz = MB(128);

        RHI_Buffer_Desc desc = {};
        desc.memory_type = RHI_MEMORY_UPLOAD;
        desc.size        = sz;

        Assert(rhi_buffer_init(device, &upload_buffer, &desc, NULL));
    }

    RHI_Semaphore upload_semaphore = {};
    u64 upload_semaphore_value = 1;
    {
        Assert(rhi_semaphore_create(device, &upload_semaphore));
    }

    RHI_Buffer vertex_buffer           = {};
    RHI_Buffer_View vertex_buffer_view = {};
    RHI_Buffer index_buffer            = {};


    { // Create vertex buffer and view
        u64 sz = sizeof(vertices);

        RHI_Buffer_Desc desc = {};
        desc.memory_type = RHI_MEMORY_GPU_ONLY;
        desc.size        = sz;

        Assert(rhi_buffer_init(device, &vertex_buffer, &desc, NULL));

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

    { // Create index buffer
        u64 sz = sizeof(indices);

        RHI_Buffer_Desc desc = {};
        desc.memory_type = RHI_MEMORY_GPU_ONLY;
        desc.size        = sz;

        Assert(rhi_buffer_init(device, &index_buffer, &desc, NULL));
    }

    { // Upload vertex buffer and index buffer.
        void *ptr = NULL;

        u64 vb_sz = sizeof(vertices);
        ptr = rhi_buffer_map(&upload_buffer);
        memcpy(ptr, vertices, vb_sz);

        u64 ib_sz = sizeof(indices);
        ptr = rhi_buffer_map(&upload_buffer);
        memcpy((u8 *)ptr + vb_sz, indices, ib_sz);

        rhi_buffer_unmap(&upload_buffer);

        rhi_command_buffer_begin(copy_buffer);
        {
            rhi_cmd_copy_buffer_to_buffer(copy_buffer, &vertex_buffer, &upload_buffer, 0, 0, vb_sz);
            rhi_cmd_copy_buffer_to_buffer(copy_buffer, &index_buffer, &upload_buffer, 0, vb_sz, ib_sz);
        }
        rhi_command_buffer_end(copy_buffer);
        rhi_submit(device, 1, &copy_buffer);
        rhi_semaphore_signal(device, RHI_COMMAND_TYPE_TRANSFER, &upload_semaphore, upload_semaphore_value);

        // @Todo: This might be the worst case synchronization. Full stop on thread 
        // until the upload is finished? Sounds terrible.
        rhi_semaphore_wait(&upload_semaphore, upload_semaphore_value, RHI_INFINITE);
        upload_semaphore_value += 1;
    }


    // Load image
    String contents = read_entire_file(S("C:/Users/swl/Desktop/doggo.png"), tctx.allocator);
    Bitmap bitmap = bitmap_import(contents.str, contents.len);

    // Create texture
    auto *tex = alloc_t(RHI_Texture);
    RHI_Texture_View tex_view = {};
    {
        // Import options? And do I want intermediate format? I think I just 
        // want to straight convert to RHI texture.
        // 1. To texture format.
        // 2. texture usage
        // 3. Mip
        RHI_Texture_Desc desc = {};
        {
            desc.type           = RHI_TEXTURE_TYPE_2D;             // @Temporary: hard-coded.
            desc.format         = RHI_TEXTURE_FORMAT_RGBA8_UNORM;  // @Temporary: hard-coded.
            desc.usage          = RHI_TEXTURE_USAGE_SAMPLED;
            desc.width          = bitmap.width;
            desc.height         = bitmap.height;
            desc.mip_levels     = 1;
            desc.depth          = 1;                               // @Temporary: hard-coded.
        }
        rhi_texture_create(device, tex, &desc, NULL);
    }
    {
        RHI_Texture_View_Desc desc = {};
        {
            desc.type               = RHI_TEXTURE_VIEW_TYPE_SAMPLED;
            desc.dimension          = RHI_TEXTURE_TYPE_2D;
            desc.format             = RHI_TEXTURE_FORMAT_RGBA8_UNORM;
            desc.base_mip_level     = 0;
            desc.base_array_layer   = 0;
            desc.mip_levels         = tex->desc.mip_levels;
            desc.depth              = tex->desc.depth;
        }
        rhi_texture_view_create(device, &tex_view, tex, &desc);
    }

    { // Upload texture
        // @Todo: I know, I know. Correct alignment for BC and other formats and 
        // RHI abstraction of D3D12 and Vulkan. Those must be resolved...
        u8 *ptr = (u8 *)rhi_buffer_map(&upload_buffer);
        u8 *dst = ptr;
        u8 *src = (u8 *)bitmap.data;
        u32 pitch = bitmap.size / bitmap.height;
        u32 aligned_pitch = align_up(pitch, 256);

        for (u32 r = 0; r < bitmap.height; ++r) {
            memcpy(dst, src, pitch);
            dst += aligned_pitch;
            src += pitch;
        }

        rhi_buffer_unmap(&upload_buffer);


        RHI_Box box = {};
        {
            box.width  = bitmap.width;
            box.height = bitmap.height;
            box.depth  = 1;
        }

        rhi_command_buffer_begin(copy_buffer);
        {
            rhi_cmd_copy_buffer_to_texture(copy_buffer, &upload_buffer, 0, aligned_pitch, tex, &box, 0, 0);
        }
        rhi_command_buffer_end(copy_buffer);
        rhi_submit(device, 1, &copy_buffer);
        rhi_semaphore_signal(device, RHI_COMMAND_TYPE_TRANSFER, &upload_semaphore, upload_semaphore_value);
        rhi_semaphore_wait(&upload_semaphore, upload_semaphore_value, RHI_INFINITE);
        upload_semaphore_value += 1;
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

            rhi_cmd_texture_barrier(cmd_buffer, &surface->textures[surface->current_frame_index], RHI_RESOURCE_STATE_COMMON, RHI_RESOURCE_STATE_RENDER_TARGET, RHI_ALL_MIPS, RHI_ALL_LAYERS);

            rhi_pass_begin(cmd_buffer, &pass);
            {
                rhi_cmd_set_pipeline(cmd_buffer, &pipeline);
                rhi_cmd_set_viewport(cmd_buffer, 0.f, 0.f, SURFACE_WIDTH, SURFACE_HEIGHT, 0.f, 1.f);
                rhi_cmd_set_scissor(cmd_buffer, 0, 0, SURFACE_WIDTH, SURFACE_HEIGHT);

                Constants constants = {};
                {
                    constants.vertex_buffer_id  = vertex_buffer_view.bindless;
                    constants.texture_id        = tex_view.bindless;
                    constants.linear_sampler_id = linear_sampler.bindless;
                }
            
                rhi_cmd_push_constants(cmd_buffer, &constants, sizeof(constants));

                rhi_cmd_draw_indexed(cmd_buffer, &index_buffer, sizeof(indices[0]), num_indices, 1, 0, 0, 0);
            }
            rhi_pass_end(cmd_buffer, &pass);

            rhi_cmd_texture_barrier(cmd_buffer, &surface->textures[surface->current_frame_index], RHI_RESOURCE_STATE_RENDER_TARGET, RHI_RESOURCE_STATE_PRESENT, RHI_ALL_MIPS, RHI_ALL_LAYERS);

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
