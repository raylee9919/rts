// Copyright Seong Woo Lee. All Rights Reserved.

#include "renderer/renderer.h"
#include "basic/context.h"
#include "basic/hash_table.h"
#include "gfx/gfx.h"
#include "math/math.h"
#include "os/os.h"
#include "profiler/profiler.h"
#include "third_party/xxhash3/xxhash.h"
#include "game.h"
#include "shared.h"
#include "shader_compiler/shader.h"

/* Render passes */
#include "pass/geometry.h"
#include "pass/postprocess.h"
#include "pass/composition.h"


/* Call 'r_init' before use. */
Renderer *renderer;

Guid                 cube_mesh;
RHI_Buffer           camera_buffer;
RHI_Buffer_View      camera_view;
void                *camera_ptr;


void game_tick(Game_State *g, f64 dt);
static void r_ring_init();
static void r_ring_deinit();


GPU_Camera gpu_camera_from_game(Camera *camera)
{
    GPU_Camera result = {};

    f32 fov = pi32 * 0.5f;
    f32 aspect_ratio = (f32)gfx->info.width / (f32)gfx->info.height;
    vec3 dir = (y_rotation(camera->yaw) * x_rotation(camera->pitch) * FORWARD_VECTOR).xyz;

    result.position  = V4(camera->position, 1.f);
    result.view      = look_to_rh(camera->position, dir, WORLD_UP);
    result.proj      = persp_fov_rh(fov, aspect_ratio, NEAR_Z, FAR_Z);
    result.view_proj = result.proj * result.view;

    return result;
}

void r_init(void *native_window_handle) 
{
    // @Temporary
    u32 width  = 1920;
    u32 height = 1080;

    { // Init GFX
        GFX_Info init = {};
        init.kind                   = RHI_KIND_D3D12;
#if BUILD_DEBUG
        init.debug                  = true;
        init.break_on_warning       = true;
#endif
        init.native_window_handle   = native_window_handle;
        init.width                  = width;
        init.height                 = height;

        init.vsync_off              = false;

        init.frame_latency_waitable = true;

        gfx_init(init, 3);
    }


    { // Allocate and construct renderer
        Allocator heap = { crt_proc, nullptr };
        renderer = (Renderer*)alloc(sizeof(Renderer), heap);
        Construct(renderer);
        renderer->heap = heap;
    }


    Renderer *r = renderer;

    { // Assign allocator
        r->passes.allocator         = r->heap;
        r->material_table.allocator = r->heap;
    }


    { // Init renderer's resources

        // Cleanup
        for (u32 i = 0; i < gfx_backbuffer_count(); ++i) 
        {
            { // SceneDepth
                r->scene_depth[i]  = guid_generate();

                RHI_Texture_Desc desc = {}; 
                desc.name           = S("SceneDepth");
                desc.type           = RHI_TEXTURE_TYPE_2D;
                desc.format         = R_DEPTH_FORMAT;
                desc.usage          = RHI_TEXTURE_USAGE_DEPTH_STENCIL_ATTACHMENT;
                desc.width          = width;
                desc.height         = height;
                desc.mip_levels     = 1;
                desc.depth          = 1;
                desc.clear          = true;
                desc.clear_depth    = 1.f;

                gfx_texture_create(r->scene_depth[i], desc);
            }

            { // GBufferColor
                r->gbuffer_color[i] = guid_generate();

                RHI_Texture_Desc desc = {};
                desc.name           = S("GBufferColor");
                desc.type           = RHI_TEXTURE_TYPE_2D;
                desc.format         = R_COLOR_FORMAT;
                desc.usage          = RHI_TEXTURE_USAGE_COLOR_ATTACHMENT | RHI_TEXTURE_USAGE_STORAGE;
                desc.width          = width;
                desc.height         = height;
                desc.mip_levels     = 1;
                desc.depth          = 1;
                desc.clear          = true;

                gfx_texture_create(r->gbuffer_color[i], desc);
            }

            { // Scene
                r->scene[i] = guid_generate();

                RHI_Texture_Desc desc = {};
                {
                    desc.name           = S("Scene");
                    desc.type           = RHI_TEXTURE_TYPE_2D;
                    desc.format         = R_COLOR_FORMAT;
                    desc.usage          = RHI_TEXTURE_USAGE_COLOR_ATTACHMENT | RHI_TEXTURE_USAGE_STORAGE;
                    desc.width          = width;
                    desc.height         = height;
                    desc.mip_levels     = 1;
                    desc.depth          = 1;
                    desc.clear          = true;
                }
                gfx_texture_create(r->scene[i], desc);
            }
        }
    }


    { // Create full-screen triangle mesh
        renderer->fullscreen_triangle_mesh  = guid_generate();
        for (int i = 0; i < 3; ++i) renderer->fullscreen_triangle_indices[i] = i;
        gfx_mesh_create(renderer->fullscreen_triangle_mesh, renderer->fullscreen_triangle_vertices, 3, sizeof(f32), renderer->fullscreen_triangle_indices, 3, sizeof(u32));
    }


    // Initialize render ring.
    r_ring_init();


    // Register render passes
    r_pass_create(RenderPassInit_Geometry);
    r_pass_create(RenderPassInit_Postprocess);
    r_pass_create(RenderPassInit_Composition);


    // Tell others the renderer is ready to communicate.
    atomic_store(&r->initted, true);
}

void r_shutdown()
{
    gfx_mesh_destroy(renderer->fullscreen_triangle_mesh);
    r_ring_deinit();
    release(renderer->heap);
}

void r_render(Game_State *g, f64 refresh_dt)
{
    ProfileScope;

    Renderer *r = renderer;

    { // Build frame graph
        // @Todo: Backbuffer index is hassle. Renderer might want to make a 
        // frame resource once and be oblivious about it.
        
        // @Cleanup
        gfx_pass_connect(r->gbuffer_color[gfx_backbuffer_index()],
                         -1, R_PASS_GEOMETRY, 
                         RHI_RESOURCE_STATE_RENDER_TARGET);

        gfx_pass_connect(r->scene_depth[gfx_backbuffer_index()], 
                         -1, R_PASS_GEOMETRY, 
                         RHI_RESOURCE_STATE_DEPTH_WRITE);

        gfx_pass_connect(r->gbuffer_color[gfx_backbuffer_index()],
                         R_PASS_GEOMETRY, R_PASS_POSTPROCESS, 
                         RHI_RESOURCE_STATE_ALL_SHADER_RESOURCE);

        gfx_pass_connect(r->scene[gfx_backbuffer_index()],
                         R_PASS_GEOMETRY, R_PASS_POSTPROCESS, 
                         RHI_RESOURCE_STATE_RENDER_TARGET);

        gfx_pass_connect(r->scene[gfx_backbuffer_index()], 
                         R_PASS_POSTPROCESS, R_PASS_COMPOSITION, 
                         RHI_RESOURCE_STATE_ALL_SHADER_RESOURCE);

        gfx_pass_connect(gfx_surface_texture(), 
                         -1, R_PASS_COMPOSITION, 
                         RHI_RESOURCE_STATE_RENDER_TARGET);
    }


    // Execute render passes.
    // Passes are sorted and "really" executed afterward.
    for (int i = 0; i < r->passes.count; ++i) 
    {
        R_Pass_Execute_Info info = {};
        info.width      = gfx->info.width;
        info.height     = gfx->info.height;
        info.game_state = g;

        R_Pass *pass = r->passes[i];
        pass->execute(pass, info);
    }


    // Sorting and submission are done here.
    gfx_end(g->time, gfx->info.vsync_off ? 0 : 1);
}

void r_entry(void *param)
{
    String name = S("RenderThread");
    thread_set_name(name);


    /* Initialize */
    r_init(param);


    /* Loop */
    // @Todo: How to properly terminate? 
    // Renderer maybe stuck in the ring buffer waiting for 
    // game thread to pass the data, where as the game thread 
    // already breaked its loop and waiting for the render thread 
    // to terminate.
    while ( !shared->should_close ) 
    {
        ProfileScopeN("RenderThreadLoop");

        auto *ring = &renderer->ring;

        mutex_lock(&ring->mutex);

        while (ring->is_empty()) {
            ProfileScopeN("RenderThreadSleepUntilWorkArrives");
            condvar_sleep(&ring->condvar, &ring->mutex, -1);
        }

        Assert(!ring->is_empty());

        Render_Entry *entry = &ring->entries[ring->read_idx];
        ring->read_idx = (ring->read_idx + 1) % array_count(ring->entries);

        { mutex_lock(&entry->mutex);

            condvar_wake_all(&ring->condvar);
            mutex_unlock(&ring->mutex);


            f64 refresh_dt = 1.0 / 120.0; // @Temporary
            r_render(entry->game_state, refresh_dt);

        } mutex_unlock(&entry->mutex);

        clear_thread_temporary_storage();
    }


    /* Cleanup */
    gfx_shutdown();
    r_shutdown();
}

Material *r_material_alloc(Guid guid) {
    return table_add(&renderer->material_table, guid, Material{});
}

void r_material_dealloc(Guid guid) {
    table_remove(&renderer->material_table, guid);
}

Material *r_material_from_guid(Guid guid) {
    Material *result = table_find_pointer(&renderer->material_table, guid);
    return result;
}

GPU_Material to_gpu_material(Material *material)  {
    GPU_Material result = {};

    result.albedo    = material->albedo;
    result.metallic  = material->metallic;
    result.roughness = material->roughness;

    result.albedo_id = gfx_srv_bindless_from_texture(material->albedo_texture);
    result.orm_id    = gfx_srv_bindless_from_texture(material->orm_texture);

    return result;
}

// @Cleanup: I don't like this a single bit.
void r_pipeline_create(Guid id,
                       String shader_filepath, 
                       String material_filepath,
                       R_Shading_Model shading_model)
{
    // Read shader source code
    String shader_source   = read_entire_file(shader_filepath, tctx.temp);
    String material_source = {};
    if (material_filepath.len) {
        material_source = read_entire_file(material_filepath, tctx.temp);
    }

    // Compile vertex and pixel shader into IL bytes.
    Shader_Compile_Result vs = {};
    Shader_Compile_Result ps = {};
    {
        Shader_Compile_Options vs_opts = {};
        {
            vs_opts.stage  = SHADER_STAGE_VS;
            vs_opts.source = shader_source;
            vs_opts.path   = shader_filepath;

            vs_opts.material_source = material_source;
            vs_opts.material_path   = material_filepath;
        }
        Assert(shader_compile(shared->shader_compiler, vs_opts, true, &vs, tctx.temp));

        Shader_Compile_Options ps_opts = {};
        {
            ps_opts.stage  = SHADER_STAGE_PS;
            ps_opts.source = shader_source;
            ps_opts.path   = shader_filepath;

            ps_opts.material_source = material_source;
            ps_opts.material_path   = material_filepath;
        }
        Assert(shader_compile(shared->shader_compiler, ps_opts, true, &ps, tctx.temp));
    }

    { // Create pipeline state object(PSO)
        bool enable_depth = r_should_enable_depth(shading_model);
        bool enable_blend = r_should_enable_blend(shading_model);

        RHI_Pipeline_Desc desc = {};
        desc.type = RHI_PIPELINE_TYPE_GRAPHICS;

        desc.depth_enabled               = enable_depth;
        desc.depth_compare_op            = RHI_COMPARE_LESS_EQUAL;
        desc.depth_format                = RHI_FORMAT_D32F;

        desc.num_color_attachments       = 1;
        {
            desc.color_attachment_formats[0] = RHI_FORMAT_RGBA16F;

            desc.blend_enabled[0]            = enable_blend;

            desc.blend_factor_color_src[0]   = RHI_BLEND_FACTOR_SRC_ALPHA;
            desc.blend_factor_color_dst[0]   = RHI_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            desc.blend_color_op[0]           = RHI_BLEND_OP_ADD;

            desc.blend_factor_alpha_src[0]   = RHI_BLEND_FACTOR_ONE;
            desc.blend_factor_alpha_dst[0]   = RHI_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            desc.blend_alpha_op[0]           = RHI_BLEND_OP_ADD;
        }

        desc.fill_mode = RHI_FILL_SOLID;
        desc.cull_mode = RHI_CULL_CW;

        desc.topology = RHI_TOPOLOGY_TRIANGLES;

        desc.vs_data = vs.data;
        desc.vs_size = vs.size;

        desc.ps_data = ps.data;
        desc.ps_size = ps.size;

        gfx_pipeline_create(id, desc);
    }
}

void r_pipeline_destroy(Guid id)
{
    gfx_pipeline_destroy(id);
}

void r_pass_create(R_Pass_Init_Proc *init_proc)
{
    R_Pass *pass = init_proc();
    array_add(&renderer->passes, pass);
}

void r_pass_destroy()
{
    // @Todo
    // Find pass with name and remove from the array.
    // Then, call the deinit proc of the pass.
}

// ------------------------------------------------------------------------- //

static void r_ring_init()
{
    Renderer *r = renderer;
    auto *ring = &r->ring;
    Construct(ring);

    mutex_create(&ring->mutex);
    condvar_create(&ring->condvar);

    for (int i = 0; i < array_count(ring->entries); ++i) {
        game_state_init(&ring->entries[i].game_state);
        mutex_create(&ring->entries[i].mutex);
    }
}

static void r_ring_deinit() 
{
    Renderer *r = renderer;
    auto *ring = &r->ring;

    mutex_destroy(&ring->mutex);
    condvar_destroy(&ring->condvar);

    for (int i = 0; i < array_count(ring->entries); ++i) {
        game_state_deinit(ring->entries[i].game_state);
        mutex_destroy(&ring->entries[i].mutex);
    }
}

// ------------------------------------------------------------------------- //
