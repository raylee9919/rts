// Copyright Seong Woo Lee. All Rights Reserved.


#include "basic/include.h"
#include "math/include.h"
#include "os/include.h"
#include "random/include.h"
#include "asset/include.h"
#include "rhi/include.h"
#include "gfx/include.h"
#include "shader/include.h"

#include "basic/include.cpp"
#include "math/include.cpp"
#include "os/include.cpp"
#include "random/include.cpp"
#include "asset/include.cpp"
#include "rhi/include.cpp"
#include "gfx/include.cpp"
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

global u64 current_frame_index = 0;

//
global Shader_Compiler     *compiler;

//
global RHI_Buffer          arguments_buffer;
global RHI_Buffer_View     arguments_view;
global void                *arguments_ptr;

//
global GFX_Texture  texture_handle;
global GFX_Pipeline pipeline;

//
struct Arguments {
    v3  position;
    u32 tint;
};


String shader_source = S(R"(
    // Copyright Seong Woo Lee. All Rights Reserved.

    float4 unpack_rgba8(uint packed) 
    {
        return float4((packed >>  0) & 0xff,
                      (packed >>  8) & 0xff,
                      (packed >> 16) & 0xff,
                      (packed >> 24) & 0xff) / 255.0;
    }

    struct Push_Constants 
    {
        uint vertex_buffer_id;
        uint texture_id;
        uint linear_sampler_id;
        uint arguments_id;
    };
    ConstantBuffer<Push_Constants> push : register(b0);

    struct Arguments 
    {
        float3 position;
        uint   tint;
    };

    struct Vertex 
    {
        float3 position;
        float2 uv;
    };
    
    struct VS_Output 
    {
        float4 sv_position : SV_POSITION;
        float3 position    : POSITION;
        float2 uv          : UV;
        float4 color       : COLOR;
    };
    
    VS_Output main_vs(uint vertex_id : SV_VertexID, uint instance_id : SV_InstanceID) 
    {
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
    
    float4 main_ps(VS_Output input) : SV_TARGET 
    {
        Texture2D <float4> color_texture  = ResourceDescriptorHeap[push.texture_id];
        SamplerState linear_sampler = SamplerDescriptorHeap[push.linear_sampler_id];
        float4 result = color_texture.Sample(linear_sampler, input.uv) * input.color;

        return result;
    }
)");


global const u64 MESH_ID     = 1219;
global const u32 RENDER_PASS = 0;

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
u32 num_vertices = array_count(vertices);

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

void proceed_game_state_ring_buffer() 
{
    game_state_index_current += 1;
    game_state_index_prev    += 1;
    game_state_index_next    += 1;

    game_state_index_current %= 3;
    game_state_index_prev    %= 3;
    game_state_index_next    %= 3;
}

void tick_game(f64 dt) 
{
    proceed_game_state_ring_buffer();

    auto *next_state    = &game_states[game_state_index_next];
    auto *current_state = &game_states[game_state_index_current];

#if 0
    for (int i = 0; i < NUM_ENTITIES; ++i) {
        f32 coef = 0.05f;
        next_state->entities[i].position.x = current_state->entities[i].position.x + dt * coef * (f32)((s32)(xorshift64() >> 32) % 32);
        next_state->entities[i].position.y = current_state->entities[i].position.y + dt * coef * (f32)((s32)(xorshift64() >> 32) % 32);

        u32 packed = (u32)xorshift64();
        next_state->entities[i].tint = unpack_rgba(packed);
    }
#else
    for (int i = 0; i < NUM_ENTITIES; ++i) {
        f32 coef = 0.05f;
        next_state->entities[i].position.x = coef * (f32)((s32)(xorshift64() >> 32) % 32);
        next_state->entities[i].position.y = coef * (f32)((s32)(xorshift64() >> 32) % 32);

        u32 packed = (xorshift32() & 0x00ffffff) | 0xA4000000;
        next_state->entities[i].tint = unpack_rgba(packed);
    }
#endif
}

// @Todo: Resolve this......!
// @Todo: Resolve this......!
// @Todo: Resolve this......!
// @Todo: Resolve this......!
// @Todo: Resolve this......!
// @Todo: Resolve this......!
// @Todo: Resolve this......!
// @Todo: Resolve this......!
// @Todo: Resolve this......!
// @Todo: Resolve this......!
void update_args(f64 alpha)
{
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

void render(f64 alpha) 
{
    gfx_pass_begin(RENDER_PASS);

    gfx_pass_color_attachment(RENDER_PASS, 0, GFX_SURFACE_TEXTURE);
    gfx_pass_clear_color(RENDER_PASS, 0xffff00ff, 0);
    gfx_pass_viewport(RENDER_PASS, 0.f, 0.f, SURFACE_WIDTH, SURFACE_HEIGHT);
    gfx_pass_scissor(RENDER_PASS, 0, 0, SURFACE_WIDTH, SURFACE_HEIGHT);

    gfx_pipeline(pipeline);

    {
        // @Temporary
        auto *mesh = table_find_pointer(&gfx->mesh_table, MESH_ID);

        // Bind root constants
        Constants c = {};
        c.vertex_buffer_id  = mesh->vertex_buffer_view.bindless;
        c.texture_id        = texture_handle.bindless[RHI_TEXTURE_VIEW_TYPE_SAMPLED];
        c.linear_sampler_id = gfx->linear_sampler.bindless;
        c.arguments_id      = arguments_view.bindless;

        gfx_push_constants(&c, sizeof(c));

        update_args(alpha);

        gfx_draw(MESH_ID, NUM_ENTITIES);
    }

    gfx_pass_end();

    gfx_end();
}

int main_entry(int argc, char **argv) 
{
    // Open window
    window = os_window_create(1920, 1080, utf8lit("rhi"));
    HWND hwnd = hwnd_from_os_handle(window);

    // Init shader compiler
    compiler = alloc_t(Shader_Compiler);
    Assert(shader_compiler_init(compiler));


    // Init GFX
    {
        GFX_Info init = {};
        init.kind                   = RHI_KIND_D3D12;
#if BUILD_DEBUG
        init.debug                  = true;
        init.break_on_warning       = true;
#endif
        init.native_window_handle   = (void *)hwnd;
        init.width                  = SURFACE_WIDTH;
        init.height                 = SURFACE_HEIGHT;
        init.num_back_buffers       = 3;

        gfx_init(init);
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


    { // Create pipeline state object(PSO)
        RHI_Pipeline_Desc desc = {};
        desc.type = RHI_PIPELINE_TYPE_GRAPHICS;

        // @Temporary
        desc.num_color_attachments       = 1;
        desc.color_attachment_formats[0] = gfx->surface->textures[0].desc.format;
        
        desc.blend_enabled[0]           = true;

        desc.blend_factor_color_src[0]  = RHI_BLEND_FACTOR_SRC_ALPHA;
        desc.blend_factor_color_dst[0]  = RHI_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        desc.blend_op_color[0]          = RHI_BLEND_OP_ADD;

        desc.blend_factor_alpha_src[0]  = RHI_BLEND_FACTOR_ONE;
        desc.blend_factor_alpha_dst[0]  = RHI_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        desc.blend_op_alpha[0]          = RHI_BLEND_OP_ADD;

        desc.fill_mode = RHI_FILL_SOLID;
        desc.cull_mode = RHI_CULL_CW;

        desc.topology = RHI_TOPOLOGY_TRIANGLES;

        desc.vs_data = vs.data;
        desc.vs_size = vs.size;

        desc.ps_data = ps.data;
        desc.ps_size = ps.size;

        pipeline = gfx_pipeline_create(desc);
    }


    gfx_mesh_create(MESH_ID, vertices, num_vertices, sizeof(vertices[0]), indices, num_indices, sizeof(indices[0]));


    // @Temporary
    auto *mesh = table_find_pointer(&gfx->mesh_table, MESH_ID);


    { // Create arguments buffer and view
        u64 stride = sizeof(Arguments);
        u64 sz     = stride * NUM_ENTITIES;

        RHI_Buffer_Desc desc = {};
        desc.memory_type = RHI_MEMORY_UPLOAD;
        desc.size        = sz;

        Assert(rhi_buffer_init(gfx->device, &arguments_buffer, &desc, NULL));

        RHI_Buffer_View_Desc view_desc = {};
        {
            view_desc.type     = RHI_BUFFER_VIEW_TYPE_STRUCTURED;
            view_desc.writable = false;
            view_desc.stride   = stride;
            view_desc.offset   = 0;
            view_desc.size     = sz;
        }

        rhi_buffer_view_init(gfx->device, &arguments_view, &arguments_buffer, &view_desc);
    }
    arguments_ptr = rhi_buffer_map(&arguments_buffer);


    // Load image
    String contents = read_entire_file(S("C:/Users/swl/Desktop/doggo.png"), tctx.allocator);
    Bitmap bitmap = bitmap_import(contents.str, contents.len);


    // Create and upload texture.
    {
        RHI_Texture_Desc tex_desc = {};
        {
            tex_desc.type           = RHI_TEXTURE_TYPE_2D;             // @Temporary: hard-coded.
            tex_desc.format         = RHI_TEXTURE_FORMAT_RGBA8_UNORM;  // @Temporary: hard-coded.
            tex_desc.usage          = RHI_TEXTURE_USAGE_SAMPLED;
            tex_desc.width          = bitmap.width;
            tex_desc.height         = bitmap.height;
            tex_desc.mip_levels     = 1;
            tex_desc.depth          = 1;                               // @Temporary: hard-coded.
        }
        texture_handle = gfx_texture_create(tex_desc);
        gfx_texture_upload(texture_handle, tex_desc.format, bitmap.data, bitmap.size, bitmap.width, bitmap.height);
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


    // Cleanup
    gfx_texture_destroy(texture_handle);
    gfx_pipeline_destroy(pipeline);
    gfx_mesh_destroy(MESH_ID);
    gfx_shutdown();


    return 0;
}
