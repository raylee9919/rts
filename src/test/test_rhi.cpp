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

//
global f64 g_time = 0.f;

struct Entity {
    v3 position;
    v4 tint;
};

#define NUM_ENTITIES 1024
struct Game_State {
    Entity entities[NUM_ENTITIES];
};

global Game_State game_states[3];
global s64 game_state_index_current = 0;
global s64 game_state_index_next    = 1;
global s64 game_state_index_prev    = 2;

//
global OS_Handle window        = {};
global b32 should_close        = false;
global u32 SURFACE_WIDTH       = 1920;
global u32 SURFACE_HEIGHT      = 1080;
global u32 num_back_buffers    = 3;
global u64 current_frame_index = 0;

//
global Shader_Compiler     *compiler;

//
global RHI_Device          *device;
global RHI_Semaphore       semaphore;
global RHI_Surface         *surface;
global RHI_Command_Buffer  *cmd_buffer;
global RHI_Command_Buffer  *compute_buffer;
global RHI_Command_Buffer  *copy_buffer;
global RHI_Texture_View    views[RHI_MAX_BACK_BUFFERS];
global RHI_Pipeline        pipeline;

global RHI_Buffer          vertex_buffer;
global RHI_Buffer_View     vertex_buffer_view;
global RHI_Buffer          index_buffer;
global RHI_Texture_View    tex_view;
global RHI_Sampler         linear_sampler;

global RHI_Buffer          arguments_buffer;
global RHI_Buffer_View     arguments_view;
global void               *arguments_ptr;


//
struct Arguments {
    v3  position;
    u32 tint;
};


String shader_source = S(R"(
    // Copyright Seong Woo Lee. All Rights Reserved.

    float4 unpack_rgba8(uint packed) {
        return float4((packed >>  0) & 0xff,
                      (packed >>  8) & 0xff,
                      (packed >> 16) & 0xff,
                      (packed >> 24) & 0xff) / 255.0;
    }

    struct Push_Constants {
        uint vertex_buffer_id;
        uint texture_id;
        uint linear_sampler_id;
        uint arguments_id;
    };
    ConstantBuffer<Push_Constants> push : register(b0);

    struct Arguments {
        float3 position;
        uint   tint;
    };

    struct Vertex {
        float3 position;
        float2 uv;
    };
    
    struct VS_Output {
        float4 sv_position : SV_POSITION;
        float3 position    : POSITION;
        float2 uv          : UV;
        float4 color       : COLOR;
    };
    
    VS_Output main_vs(uint vertex_id : SV_VertexID, uint instance_id : SV_InstanceID) {
        VS_Output result;

        StructuredBuffer<Vertex> vertex_buffer = ResourceDescriptorHeap[push.vertex_buffer_id];
        Vertex vert = vertex_buffer[vertex_id];

        StructuredBuffer<Arguments> arguments_buffer = ResourceDescriptorHeap[push.arguments_id];
        Arguments args = arguments_buffer[instance_id];
        float3 translation = args.position;

        result.sv_position = float4(vert.position + translation, 1.0);
        result.position    = vert.position + translation;
        result.uv          = vert.uv;
        result.color       = unpack_rgba8(args.tint);

        return result;
    }
    
    float4 main_ps(VS_Output input) : SV_TARGET {
        Texture2D <float4> color_texture  = ResourceDescriptorHeap[push.texture_id];
        SamplerState linear_sampler = SamplerDescriptorHeap[push.linear_sampler_id];
        float4 result = color_texture.Sample(linear_sampler, input.uv) * input.color;

        return result;
    }
)");


struct Vertex {
    v3 position;
    v2 uv;
};

Vertex vertices[] = {
    { v3(-0.125f * 0.5625f, -0.125f, 0.0f), v2(0.f, 1.f) },
    { v3( 0.125f * 0.5625f, -0.125f, 0.0f), v2(1.f, 1.f) },
    { v3(-0.125f * 0.5625f,  0.125f, 0.0f), v2(0.f, 0.f) },
    { v3( 0.125f * 0.5625f,  0.125f, 0.0f), v2(1.f, 0.f) },
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
    u32 arguments_id;
};


global u64 rng_state = 0x123456789abcdef0ULL;
u64 rng_u64()
{
    u64 x = rng_state;

    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;

    rng_state = x;
    return x;
}

v4 unpack_rgba(u32 rgba)
{
    return {
        f32((rgba >>  0) & 0xFF),
        f32((rgba >>  8) & 0xFF),
        f32((rgba >> 16) & 0xFF),
        f32((rgba >> 24) & 0xFF)
    };
}

u32 pack_rgba(v4 rgba)
{
    u32 r = u32(rgba.x * 255.0f + 0.5f);
    u32 g = u32(rgba.y * 255.0f + 0.5f);
    u32 b = u32(rgba.z * 255.0f + 0.5f);
    u32 a = u32(rgba.w * 255.0f + 0.5f);

    return (r << 0) |
           (g << 8) |
           (b << 16) |
           (a << 24);
}

void proceed_game_state_ring_buffer() {
        game_state_index_current += 1;
        game_state_index_prev    += 1;
        game_state_index_next    += 1;

        game_state_index_current %= 3;
        game_state_index_prev    %= 3;
        game_state_index_next    %= 3;
}

void tick_game(f64 dt) {
    proceed_game_state_ring_buffer();

    auto *next_state    = &game_states[game_state_index_next];
    auto *current_state = &game_states[game_state_index_current];

    for (int i = 0; i < NUM_ENTITIES; ++i) {
        f32 coef = 0.05f;
        next_state->entities[i].position.x = current_state->entities[i].position.x + dt * coef * (f32)((s32)(rng_u64() >> 32) % 32);
        next_state->entities[i].position.y = current_state->entities[i].position.y + dt * coef * (f32)((s32)(rng_u64() >> 32) % 32);

        u32 packed = (u32)rng_u64();
        next_state->entities[i].tint = unpack_rgba(packed);
    }
}

void render(f64 alpha) 
{
    rhi_command_buffer_begin(cmd_buffer);

    RHI_Pass pass = {};
    {
        pass.name = S("Textured Quad");
        pass.num_color_attachments = 1;

        auto *attachment = &pass.color_attachments[0];
        {
            attachment->view           = views[surface->current_frame_index];
            attachment->load_op        = RHI_LOAD_OP_CLEAR;
            attachment->clear_color[0] = 0.3f;
            attachment->clear_color[1] = 0.2f;
            attachment->clear_color[2] = 0.2f;
            attachment->clear_color[3] = 1.0f;
        }
    }

    rhi_cmd_texture_barrier(cmd_buffer, &surface->textures[surface->current_frame_index], RHI_RESOURCE_STATE_COMMON, RHI_RESOURCE_STATE_RENDER_TARGET, RHI_ALL_MIPS, RHI_ALL_LAYERS);

    rhi_pass_begin(cmd_buffer, &pass);
    {
        // Set states
        rhi_cmd_set_pipeline(cmd_buffer, &pipeline);
        rhi_cmd_set_viewport(cmd_buffer, 0.f, 0.f, SURFACE_WIDTH, SURFACE_HEIGHT, 0.f, 1.f);
        rhi_cmd_set_scissor(cmd_buffer, 0, 0, SURFACE_WIDTH, SURFACE_HEIGHT);


        // Bind root constants
        Constants constants = {};
        {
            constants.vertex_buffer_id  = vertex_buffer_view.bindless;
            constants.texture_id        = tex_view.bindless;
            constants.linear_sampler_id = linear_sampler.bindless;
            constants.arguments_id      = arguments_view.bindless;
        }
        rhi_cmd_push_constants(cmd_buffer, &constants, sizeof(constants));


        { // Update arguments
            auto *current = &game_states[game_state_index_current];
            auto *next    = &game_states[game_state_index_next];

            for (u32 i = 0; i < NUM_ENTITIES; ++i) {
                auto *E1 = &current->entities[i];
                auto *E2 = &next->entities[i];

                // Interpolate, @Todo: Turns out, there's a better way?
                v3 position = lerp(E1->position, alpha, E2->position);
                v4 tint     = lerp(E1->tint, alpha, E2->tint);

                Arguments *args = (Arguments *)arguments_ptr + i;
                args->position = position; 
                args->tint = pack_rgba(tint);
            }
        }


        // Draw
        u32 num_instances = NUM_ENTITIES;
        rhi_cmd_draw_indexed(cmd_buffer, &index_buffer, sizeof(indices[0]), num_indices, num_instances, 0, 0, 0);
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

int main_entry(int argc, char **argv) 
{
    // Open window
    window = os_window_create(1920, 1080, utf8lit("rhi"));
    HWND hwnd = hwnd_from_os_handle(window);

    // Init shader compiler
    compiler = alloc_t(Shader_Compiler);
    Assert(shader_compiler_init(compiler));



    device = alloc_t(RHI_Device);
    Assert(rhi_device_init(device, RHI_KIND_D3D12, true, true));

    Assert(rhi_semaphore_init(device, &semaphore));

    surface = alloc_t(RHI_Surface);
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


    for (u32 i = 0; i < RHI_MAX_BACK_BUFFERS; ++i) {
        RHI_Texture_View_Desc view_desc = {};
        {
            view_desc.type              = RHI_TEXTURE_VIEW_TYPE_RENDER_TARGET;
            view_desc.dimension         = RHI_TEXTURE_TYPE_2D;
            view_desc.format            = surface->textures[i].desc.format;
            view_desc.base_mip_level    = 0;
        }
        rhi_texture_view_init(device, &views[i], &surface->textures[i], &view_desc);
    }


    cmd_buffer = alloc_t(RHI_Command_Buffer);
    Assert(rhi_command_buffer_init(device, cmd_buffer, RHI_COMMAND_TYPE_GRAPHICS));

    compute_buffer = alloc_t(RHI_Command_Buffer);
    Assert(rhi_command_buffer_init(device, compute_buffer, RHI_COMMAND_TYPE_COMPUTE));

    copy_buffer = alloc_t(RHI_Command_Buffer);
    Assert(rhi_command_buffer_init(device, copy_buffer, RHI_COMMAND_TYPE_TRANSFER));


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
        desc.color_attachment_formats[0] = surface->textures[0].desc.format;

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
        Assert(rhi_semaphore_init(device, &upload_semaphore));
    }


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


    { // Create arguments buffer and view
        u64 stride = sizeof(Arguments);
        u64 sz     = stride * NUM_ENTITIES;

        RHI_Buffer_Desc desc = {};
        desc.memory_type = RHI_MEMORY_UPLOAD;
        desc.size        = sz;

        Assert(rhi_buffer_init(device, &arguments_buffer, &desc, NULL));

        RHI_Buffer_View_Desc view_desc = {};
        {
            view_desc.type     = RHI_BUFFER_VIEW_TYPE_STRUCTURED;
            view_desc.writable = false;
            view_desc.stride   = stride;
            view_desc.offset   = 0;
            view_desc.size     = sz;
        }

        rhi_buffer_view_init(device, &arguments_view, &arguments_buffer, &view_desc);
    }
    arguments_ptr = rhi_buffer_map(&arguments_buffer);


    // Load image
    String contents = read_entire_file(S("C:/Users/swl/Desktop/doggo.png"), tctx.allocator);
    Bitmap bitmap = bitmap_import(contents.str, contents.len);

    // Create texture
    auto *tex = alloc_t(RHI_Texture);
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
        rhi_texture_init(device, tex, &desc, NULL);
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
        rhi_texture_view_init(device, &tex_view, tex, &desc);
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


    u64 counter_prev = os_counter();
    f64 dt = 1.f / 60.f; // Update frequency, Tick rate
    f64 accumulator = 0.f;


    while (!should_close) {
        // Clear and poll events.
        os_clear_events();
        os_poll_events(); // @Todo: Timestep

        // Close app if needed
        list_for(os->first_event, event) {
            b32 esc_pressed            = event->kind == OS_EVENT_PRESS && event->key == KEY_ESC;
            b32 alt_f4_pressed         = event->kind == OS_EVENT_PRESS && event->key == KEY_F4 && (event->modifiers & OS_MODIFIER_ALT);
            b32 window_close_triggered = event->kind == OS_EVENT_WINDOW_CLOSE && event->window == window;

            if (esc_pressed || alt_f4_pressed | window_close_triggered) {
                should_close = true;
                os_remove_event(event);
            }

            // Fullscreen
            b32 alt_enter_pressed = event->kind == OS_EVENT_PRESS && event->key == KEY_RETURN && (event->modifiers & OS_MODIFIER_ALT);
            if (alt_enter_pressed) {
                os_window_toggle_fullscreen(window);
            }
        }


        // Time
        u64 counter_now     = os_counter();
        u64 counter_elapsed = counter_now - counter_prev;
        f64 time_elapsed    = (f64)counter_elapsed / (f64)os_counter_freq();
        counter_prev        = counter_now;

        g_time      += time_elapsed; 
        accumulator += time_elapsed;


        // Update
        while (accumulator >= dt) {
            accumulator -= dt;
            tick_game(dt);
        }

        f64 alpha = accumulator / dt;
        render(alpha);

        clear_temporary_storage();
    }

    rhi_command_buffer_deinit(cmd_buffer);
    rhi_device_deinit(device);

    return 0;
}
