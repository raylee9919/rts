// Copyright Seong Woo Lee. All Rights Reserved.


#include "profiler/include.h"

#include "basic/include.h"
#include "math/include.h"
#include "os/include.h"
#include "random/include.h"
#include "geometry/include.h"
#include "asset/include.h"
#include "rhi/include.h"
#include "gfx/include.h"
#include "shader/include.h"

#include "basic/include.cpp"
#include "math/include.cpp"
#include "os/include.cpp"
#include "random/include.cpp"
#include "geometry/include.cpp"
#include "asset/include.cpp"
#include "rhi/include.cpp"
#include "gfx/include.cpp"
#include "shader/include.cpp"

//
#define WORLD_UP    v3{0.f, 1.f, 0.f}
#define NEAR_Z      1e-3f
#define FAR_Z       1e9f
global f64 g_time = 0.f;

struct Entity {
    v3 position;

    Guid material;
};

struct Camera {
    v3  position;
    f32 yaw;
    f32 pitch;
};

#define NUM_ENTITIES 64
struct Game_State {
    Entity entities[NUM_ENTITIES];
    Camera camera;
};

#define GAME_STATE_MAIN_INDEX   0
#define GAME_STATE_RENDER_INDEX 1
global Game_State game_states[2];

//
struct R_Camera {
    v4   position;
    m4x4 view;
    m4x4 proj;
    m4x4 view_proj;
};
global RHI_Buffer          camera_buffer;
global RHI_Buffer_View     camera_view;
global void                *camera_ptr;


//
global OS_Handle window        = {};
global b32 should_close        = false;
global u32 SURFACE_WIDTH       = 1920;
global u32 SURFACE_HEIGHT      = 1080;
global f32 VIEWPORT_WIDTH      = 1920.f;
global f32 VIEWPORT_HEIGHT     = 1080.f;

//
global Shader_Compiler     *compiler;

//
global RHI_Buffer          arguments_buffer;
global RHI_Buffer_View     arguments_view;
global void                *arguments_ptr;

//
#define MAX_MATERIALS      1024
global RHI_Buffer          material_buffer;
global RHI_Buffer_View     material_view;
global void                *material_ptr;

//
global Guid         depth_texture;
global Guid         doggo_texture;
global GFX_Pipeline pipeline;

global const u64 MESH_ID     = 1219;
global const u32 RENDER_PASS = 0;

struct Vertex {
    v3 position;
    v3 normal;
    v2 uv;
};

Vertex vertices[24];
u32 indices[36];

u32 num_vertices = array_count(vertices);
u32 num_indices  = array_count(indices);

void tick_game(s64 index, f64 dt) {
    auto *state = &game_states[index];

    for (int i = 0; i < NUM_ENTITIES; ++i) {
        auto *E = &state->entities[i];

        f32 coef = 30.0f;
        f32 idx = (f32)i;
#if 1
        int GRID_WIDTH = 10;
        f32 SPACING = 4.0f;

        // Weird ordering.
        u32 cell = (u32)i * 2654435761u;
        cell %= NUM_ENTITIES;

        int x = cell % GRID_WIDTH;
        int z = cell / GRID_WIDTH;

        E->position.x = ((f32)x - (GRID_WIDTH - 1) * 0.5f) * SPACING;
        E->position.y = 0.0f;
        E->position.z = ((f32)z - (GRID_WIDTH - 1) * 0.5f) * SPACING;
#else
        E->position.z = coef * m_cos(m_cos(g_time) + idx * (pi32 * 2.0f / (f32)NUM_ENTITIES));
        E->position.x = coef * m_sin(m_cos(g_time) + idx * (pi32 * 2.0f / (f32)NUM_ENTITIES));
#endif
    }
}

void render(s64 index) 
{
    gfx_pass_begin(RENDER_PASS);
    {
        gfx_pass_color_attachment(RENDER_PASS, 0, GFX_SURFACE_TEXTURE);
        gfx_pass_clear_color(RENDER_PASS, 0xff201010, 0);

        gfx_pass_depth_attachment(RENDER_PASS, depth_texture);
        gfx_pass_clear_depth(RENDER_PASS, 1.f);

        gfx_set_viewport(0.f, 0.f, SURFACE_WIDTH, SURFACE_HEIGHT);
        gfx_set_scissor(0, 0, SURFACE_WIDTH, SURFACE_HEIGHT);

        gfx_set_pipeline(pipeline);

        {
            // @Temporary
            auto *mesh = table_find_pointer(&gfx->mesh_table, MESH_ID);

            // Bind root constants
            Constants c = {};
            c.vertex_buffer_id   = mesh->vertex_buffer_view.bindless;
            c.linear_sampler_id  = gfx->linear_sampler.bindless;
            c.camera_id          = camera_view.bindless;
            c.arguments_id       = arguments_view.bindless;
            c.material_buffer_id = material_view.bindless;

            gfx_push_constants(&c, sizeof(c));

            {
                auto *state = &game_states[index];

                for (u32 i = 0; i < NUM_ENTITIES; ++i) {
                    auto *E = &state->entities[i];

                    Arguments *args = (Arguments *)arguments_ptr + i;

                    memcpy(&args->position, &E->position, sizeof(args->position));

                    m4x4 m = identity();
                    memcpy(&args->orientation, &m, sizeof(args->orientation));

                    GFX_Material *mat    = gfx_material_pointer_from_guid(E->material);
                    Shader_Material sm   = gfx_to_shader_material(mat);
                    Shader_Material *dst = (Shader_Material *)material_ptr + i;
                    memcpy(dst, &sm, sizeof(sm));
                    args->material_id = i;
                }

                R_Camera camera = {};
                f32 fov = pi32 * 0.5f;
                f32 aspect_ratio = (f32)VIEWPORT_WIDTH / (f32)VIEWPORT_HEIGHT;
                camera.position  = V4(state->camera.position, 1.f);
                camera.view      = look_at_rh(state->camera.position, v3{}, WORLD_UP);
                camera.proj      = persp_fov_rh(fov, aspect_ratio, NEAR_Z, FAR_Z);
                camera.view_proj = camera.proj * camera.view;
                memcpy(camera_ptr, &camera, sizeof(camera));
            }

            gfx_draw(MESH_ID, NUM_ENTITIES);
        }
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

    // @Temporary: Init camera
    game_states[GAME_STATE_MAIN_INDEX].camera.position = v3(0.f, 6.f, 15.f);

    // Make a cube
    geo_make_cube(vertices, sizeof(Vertex), offset_of(Vertex, position), offset_of(Vertex, normal), offset_of(Vertex, uv), indices, sizeof(indices[0]));

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

        init.num_buffers            = 3;
        init.num_frames             = 2;

        gfx_init(init);
    }

    String shader_source = read_entire_file(S("../src/test/shader.hlsl"), tctx.allocator);

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

        desc.depth_enabled              = true;
        desc.depth_compare_op           = RHI_COMPARE_LESS_EQUAL;
        desc.depth_format               = RHI_TEXTURE_FORMAT_D32F;

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

    { // Create material buffer and view.
        u64 stride = sizeof(Shader_Material);
        u64 sz     = stride * MAX_MATERIALS;

        RHI_Buffer_Desc desc = {};
        desc.memory_type = RHI_MEMORY_UPLOAD;
        desc.size        = sz;

        Assert(rhi_buffer_init(gfx->device, &material_buffer, &desc, NULL));

        RHI_Buffer_View_Desc view_desc = {};
        {
            view_desc.type     = RHI_BUFFER_VIEW_TYPE_STRUCTURED;
            view_desc.writable = false;
            view_desc.stride   = stride;
            view_desc.offset   = 0;
            view_desc.size     = sz;
        }

        rhi_buffer_view_init(gfx->device, &material_view, &material_buffer, &view_desc);

        material_ptr = rhi_buffer_map(&material_buffer);
    }

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

        arguments_ptr = rhi_buffer_map(&arguments_buffer);
    }

    { // Create camera buffer and view
        u64 stride = sizeof(R_Camera);
        u64 sz     = sizeof(R_Camera) * 1;

        RHI_Buffer_Desc desc = {};
        desc.memory_type = RHI_MEMORY_UPLOAD;
        desc.size        = sz;

        Assert(rhi_buffer_init(gfx->device, &camera_buffer, &desc, NULL));

        RHI_Buffer_View_Desc view_desc = {};
        {
            view_desc.type     = RHI_BUFFER_VIEW_TYPE_STRUCTURED;
            view_desc.writable = false;
            view_desc.stride   = stride;
            view_desc.offset   = 0;
            view_desc.size     = sz;
        }

        rhi_buffer_view_init(gfx->device, &camera_view, &camera_buffer, &view_desc);

        camera_ptr = rhi_buffer_map(&camera_buffer);
    }


    // Load image
    Guid doggo_guid = guid_generate();
    String contents = read_entire_file(S("C:/Users/swl/Desktop/doggo.png"), tctx.allocator);
    Bitmap bitmap   = bitmap_import(contents.str, contents.len);


    // Create and upload texture.
    {
        RHI_Texture_Desc desc = {};
        {
            desc.type           = RHI_TEXTURE_TYPE_2D;
            desc.format         = RHI_TEXTURE_FORMAT_RGBA8_UNORM;
            desc.usage          = RHI_TEXTURE_USAGE_SAMPLED;
            desc.width          = bitmap.width;
            desc.height         = bitmap.height;
            desc.mip_levels     = 1;
            desc.depth          = 1;
        }
        doggo_guid = guid_generate();
        gfx_texture_create(doggo_guid, desc);
        gfx_texture_upload(doggo_guid, desc.format, bitmap.data, bitmap.size, bitmap.width, bitmap.height);
    }

    // Create detph texture.
    {
        RHI_Texture_Desc desc = {};
        {
            desc.type           = RHI_TEXTURE_TYPE_2D;
            desc.format         = RHI_TEXTURE_FORMAT_D32F;
            desc.usage          = RHI_TEXTURE_USAGE_DEPTH_STENCIL_ATTACHMENT;
            desc.width          = SURFACE_WIDTH;
            desc.height         = SURFACE_HEIGHT;
            desc.mip_levels     = 1;
            desc.depth          = 1;
            desc.clear          = true;
            desc.clear_depth    = 1.f;
        }
        depth_texture = guid_generate();
        gfx_texture_create(depth_texture, desc);
    }
 

    u64 counter_prev = os_counter();
    f64 dt = 1.f / 60.f; // Update frequency, Tick rate
    f64 accumulator = 0.f;

    auto *state = &game_states[GAME_STATE_MAIN_INDEX];
    for (int i = 0; i < NUM_ENTITIES; ++i) {
        auto *E = &state->entities[i];

        Guid guid = guid_generate();
        E->material = guid;

        GFX_Material material = {};
        material.albedo = doggo_guid;
        material.tint   = 0x22ffffff;

        // ..or read from asset pack.

        gfx_material_alloc(guid, material);
    }

    while (!should_close) {
        ProfileScopeN("main_loop");
        //
        // @Important: WaitForSwapchain() must be go before ProcessInput().
        // (https://unity.com/blog/engine-platform/fixing-time-deltatime-in-unity-2020-2-for-smoother-gameplay)
        //

        // @Temporary
        // Wait for display image and get timestamp.
        DXGI_FRAME_STATISTICS stats = {};
        gfx->surface->d3d12.swap_chain_4->GetFrameStatistics(&stats);

        // Time
        u64 counter_now     = os_counter();
        u64 counter_elapsed = counter_now - counter_prev;
        f64 time_elapsed    = (f64)counter_elapsed / (f64)os_counter_freq();
        counter_prev        = counter_now;
        g_time             += time_elapsed; 
        accumulator        += time_elapsed;

        // Clear and poll events.
        os_clear_events();
        os_poll_events();

        // Update
        for (u32 counter = 0; accumulator >= dt && counter < 10; counter += 1) {
            accumulator -= dt;
            tick_game(GAME_STATE_MAIN_INDEX, dt);
        }
        memcpy(&game_states[GAME_STATE_RENDER_INDEX], &game_states[GAME_STATE_MAIN_INDEX], sizeof(Game_State));
        tick_game(GAME_STATE_RENDER_INDEX, accumulator);

        // Render
        {
            ProfileScopeN("render");
            render(GAME_STATE_RENDER_INDEX);
        }

        // Close app if needed
        list_for(os->first_event, event) 
        {
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

        // Clear temporary storage
        clear_temporary_storage();
    }


    // Cleanup
    gfx_texture_destroy(doggo_texture);
    gfx_pipeline_destroy(pipeline);
    gfx_mesh_destroy(MESH_ID);
    gfx_shutdown();


    return 0;
}
