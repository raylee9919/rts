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
#include "renderer/include.h"
#include "shader_compiler/include.h"

#include "basic/include.cpp"
#include "math/include.cpp"
#include "os/include.cpp"
#include "random/include.cpp"
#include "geometry/include.cpp"
#include "asset/include.cpp"
#include "rhi/include.cpp"
#include "gfx/include.cpp"
#include "renderer/include.cpp"
#include "shader_compiler/include.cpp"

enum Input_Action : u16 {
    IA_CAMERA_MOVE_FORWARD,
    IA_CAMERA_MOVE_BACKWARD,
    IA_CAMERA_MOVE_RIGHT,
    IA_CAMERA_MOVE_LEFT,
};

//
#define WORLD_UP            v3{ 0.f,  1.f,  0.f}
#define FORWARD_VECTOR      v4{ 0.f,  0.f, -1.f, 1.f}
#define RIGHT_VECTOR        v4{ 1.f,  0.f,  0.f, 1.f}
#define UP_VECTOR           v4{ 0.f,  1.f,  0.f, 1.f}
#define NEAR_Z              1e-3f
#define FAR_Z               1e9f
#define NUM_ENTITIES        64
#define MAX_MATERIALS       1024

struct Entity {
    v3 position;

    Guid material;
};

struct Camera {
    v3  position;
    f32 yaw;
    f32 pitch;
};

struct Game_State {
    f64     time;
    Entity  entities[NUM_ENTITIES];
    Camera  camera;
};

global Game_State game_state;
global Game_State render_game_state;

//
global RHI_Buffer          camera_buffer;
global RHI_Buffer_View     camera_view;
global void                *camera_ptr;


//
global OS_Handle window        = {};
global b32 should_close        = false;
global f32 VIEWPORT_WIDTH      = 1920.f;
global f32 VIEWPORT_HEIGHT     = 1080.f;


//
global Shader_Compiler     *compiler;

//
global RHI_Buffer          arguments_buffer;
global RHI_Buffer_View     arguments_view;
global void                *arguments_ptr;

//
global RHI_Buffer          material_buffer;
global RHI_Buffer_View     material_view;
global void                *material_ptr;

//
global Guid         doggo_guid;
global Guid         pipeline;

global Guid         cube_mesh;

struct Vertex {
    v3 position;
    v3 normal;
    v2 uv;
};

Vertex vertices[24];
u32 indices[36];

u32 num_vertices = array_count(vertices);
u32 num_indices  = array_count(indices);

internal GPU_Camera gpu_camera_from_game(Camera *camera) {
    GPU_Camera result = {};

    f32 fov = pi32 * 0.5f;
    f32 aspect_ratio = (f32)VIEWPORT_WIDTH / (f32)VIEWPORT_HEIGHT;
    v3 dir = (y_rotation(camera->yaw) * x_rotation(camera->pitch) * FORWARD_VECTOR).xyz;

    result.position  = V4(camera->position, 1.f);
    result.view      = look_to_rh(camera->position, dir, WORLD_UP);
    result.proj      = persp_fov_rh(fov, aspect_ratio, NEAR_Z, FAR_Z);
    result.view_proj = result.proj * result.view;

    return result;
}

void tick_game(Game_State *state, f64 dt) {
    state->time += dt;

    {
        Camera *camera = &game_state.camera;

        f32 movement_speed = 10.f;
        f32 turn_speed     = 0.25f;

        static v2 p0 = {};

        list_for(os->first_event, event) {
            if (event->kind == OS_EVENT_PRESS && event->key == KEY_MOUSE_LEFT && event->window == window) {
                os_remove_event(event);
                p0 = event->position;
            }
        }

        if (os->key_is_down[KEY_MOUSE_LEFT]) {
            v2 p1 = os_get_mouse_position(window);
            v2 dp = p1 - p0;

            camera->yaw   -= dt * turn_speed * dp.x;
            camera->pitch -= dt * turn_speed * dp.y;
            camera->pitch = clamp(camera->pitch,  -pi32 * 0.25f, pi32 * 0.25f);

            p0 = p1;
        }

        if (os->key_is_down[KEY_W]) {
            camera->position += dt * movement_speed * (x_rotation(camera->pitch) * y_rotation(camera->yaw) * FORWARD_VECTOR).xyz;
        }

        if (os->key_is_down[KEY_S]) {
            camera->position -= dt * movement_speed * (x_rotation(camera->pitch) * y_rotation(camera->yaw) * FORWARD_VECTOR).xyz;
        }

        if (os->key_is_down[KEY_A]) {
            camera->position -= dt * movement_speed * (y_rotation(camera->yaw) * RIGHT_VECTOR).xyz;
        }

        if (os->key_is_down[KEY_D]) {
            camera->position += dt * movement_speed * (y_rotation(camera->yaw) * RIGHT_VECTOR).xyz;
        }

        if (os->key_is_down[KEY_E]) {
            camera->position += dt * movement_speed * UP_VECTOR.xyz;
        }

        if (os->key_is_down[KEY_Q]) {
            camera->position -= dt * movement_speed * UP_VECTOR.xyz;
        }
    }

    for (int i = 0; i < NUM_ENTITIES; ++i) {
        auto *E = &state->entities[i];

        f32 coef = 30.0f;
        f32 idx = (f32)i;

        E->position.z = coef * m_cos(idx * (pi32 * 2.0f / (f32)NUM_ENTITIES));
        E->position.x = coef * m_sin(idx * (pi32 * 2.0f / (f32)NUM_ENTITIES));
    }
}

int main_entry(int argc, char **argv)
{
    // Open window
    window = os_window_create(1920, 1080, S("RHI"));

    // Launch render thread
    Thread render_thread = thread_launch(render_entry, get_native_window_handle(window));

    // Init shader compiler
    compiler = alloc_t(Shader_Compiler);
    Assert(shader_compiler_init(compiler));
    compiler->include_path = S("C:\\dev\\rts\\src\\shaders\\");

    // Spin-lock until gfx is initted.
    while (!gfx || !gfx->initted) { _mm_pause(); }


    // @Temporary: Init camera
    game_state.camera.position = v3(0.f, 6.f, 15.f);

    // Make a cube
    cube_mesh = guid_generate();
    geo_make_cube(vertices, sizeof(Vertex), offset_of(Vertex, position), offset_of(Vertex, normal), offset_of(Vertex, uv), indices, sizeof(indices[0]));


    u64 counter_prev = os_counter();
    f64 dt = 1.f / 60.f; // Update frequency, Tick rate
    f64 base_framerate = 1.0 / 60.0;
    f64 accumulator = 0.f;


    {
        String shader_source = read_entire_file(S("../src/shaders/shader.hlsl"), tctx.allocator);

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
            desc.depth_format               = RHI_FORMAT_D32F;

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

            pipeline = guid_generate();
            gfx_pipeline_create(pipeline, desc);
        }


        gfx_mesh_create(cube_mesh, vertices, num_vertices, sizeof(vertices[0]), indices, num_indices, sizeof(indices[0]));


        // @Temporary
        auto *mesh = table_find_pointer(&gfx->mesh_table, cube_mesh);

        { // Create material buffer and view.
            u64 stride = sizeof(GPU_Material);
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
            u64 stride = sizeof(GPU_Camera);
            u64 sz     = sizeof(GPU_Camera) * 1;

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
        doggo_guid = guid_generate();
        String contents = read_entire_file(S("C:/Users/swl/Desktop/doggo.png"), tctx.allocator);
        Bitmap bitmap   = bitmap_import(contents.str, contents.len);


        // Create and upload texture.
        {
            RHI_Texture_Desc desc = {};
            {
                desc.type           = RHI_TEXTURE_TYPE_2D;
                desc.format         = RHI_FORMAT_RGBA8_UNORM;
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
    }


    for (int i = 0; i < NUM_ENTITIES; ++i) {
        Entity *E = &game_state.entities[i];

        Guid guid = guid_generate();
        E->material = guid;

        GFX_Material material = {};
        material.albedo = unpack_rgba(xorshift32()).xyz;
        material.albedo_texture = doggo_guid;

        // ..or read from asset pack.

        gfx_material_alloc(guid, material);
    }

    while (!should_close) {
        ProfileScopeN("main_loop");

        // Time
        u64 counter_now     = os_counter();
        u64 counter_elapsed = counter_now - counter_prev;
        f64 time_elapsed    = (f64)counter_elapsed / (f64)os_counter_freq();
        counter_prev        = counter_now;
        accumulator        += time_elapsed;

        {
            ProfileScopeN("InputProcessing");
            // Clear and poll events.
            os_clear_events();
            os_poll_events();
        }

        // Update
        memcpy(&render_game_state, &game_state, sizeof(Game_State));

        for (u32 counter = 0; accumulator >= dt && counter < 10; counter += 1) {
            accumulator -= dt;
            tick_game(&game_state, dt);
        }

        tick_game(&render_game_state, accumulator);


        // @Todo: Copy game state ... how?


        // Spinlock until the ITC render queue has a room.
        {
            ProfileScopeN("SpinlockOnRender_ITC_SPSC_Queue");
            while (render_queue.is_full()) { _mm_pause(); }
        }
            
            
        Render_Entry entry = {};

        render_queue.push(entry);


        // Close app if needed
        list_for(os->first_event, event) 
        {
            b32 esc_pressed            = event->kind == OS_EVENT_PRESS && event->key == KEY_ESC;
            b32 alt_f4_pressed         = event->kind == OS_EVENT_PRESS && event->key == KEY_F4 && (event->modifiers & OS_MODIFIER_ALT);
            b32 window_close_triggered = event->kind == OS_EVENT_WINDOW_CLOSE && event->window == window;

            if (esc_pressed || alt_f4_pressed | window_close_triggered) {
                should_close = true;
                gfx->should_shutdown = true;
                os_remove_event(event);
            }

            // Fullscreen
            b32 alt_enter_pressed = event->kind == OS_EVENT_PRESS && event->key == KEY_RETURN && (event->modifiers & OS_MODIFIER_ALT);
            if (alt_enter_pressed) {
                os_window_toggle_fullscreen(window);
            }
        }

        clear_temporary_storage();
    }

    return 0;
}
