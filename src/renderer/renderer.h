// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RTS_RENDERER_H
#define RTS_RENDERER_H

#include "basic/core.h"
#include "os/os.h"
#include "rhi/rhi.h"
#include "gfx/gfx.h"
#include "shaders/shared/shared.h"

#define R_DEPTH_FORMAT  RHI_FORMAT_D32F
#define R_COLOR_FORMAT  RHI_FORMAT_RGBA16F

struct Game_State;
struct Camera;
struct R_Pass;


// ------------------------------------------------------------------------- //


struct R_Pass_Execute_Info {
    // Portion of swapchain to draw. Will be used at blting.
    f32         x, y, w, h;

    // Resolution users select from the option
    u32         resolution_x;
    u32         resolution_y;

    Game_State *game_state;
};

#define R_PASS_DECLARE_PROCS( Name ) \
    R_PASS_INIT( RenderPassInit_##Name ); \
    R_PASS_DEINIT( RenderPassDeinit_##Name ); \
    R_PASS_EXECUTE( RenderPassExecute_##Name )

#define R_PASS_INIT( Name ) R_Pass *Name( void )
typedef R_PASS_INIT( R_Pass_Init_Proc );

#define R_PASS_DEINIT( Name ) void Name( R_Pass *inPass )
typedef R_PASS_DEINIT( R_Pass_Deinit_Proc );

#define R_PASS_EXECUTE( Name ) void Name( R_Pass *inPass, R_Pass_Execute_Info inInfo )
typedef R_PASS_EXECUTE( R_Pass_Execute_Proc );


struct R_Pass 
{
    String name;

    R_Pass_Init_Proc    *init;
    R_Pass_Deinit_Proc  *deinit;
    R_Pass_Execute_Proc *execute;
};

void r_pass_create(R_Pass_Init_Proc *init_proc);
void r_pass_destroy();


// ------------------------------------------------------------------------- //


enum Render_Pass : u32 {
    R_PASS_GEOMETRY    = 0,
    R_PASS_POSTPROCESS = 1,
    R_PASS_COMPOSITION = 2,
};

struct Render_Entry {
    Mutex           mutex;
    Game_State     *game_state;
};

struct Render_SPSC_Queue {
    Mutex           mutex      = {};
    Condvar         condvar    = {};

    Render_Entry    entries[3] = {}; // cap = 2
    s32             read_idx   = 0;
    s32             write_idx  = 0;


    b32 is_empty() {
        return read_idx == write_idx;
    }

    b32 is_full() {
        return (write_idx + 1) % array_count(entries) == read_idx;
    }
};


enum R_Shading_Model {
    SHADING_MODEL_OPAQUE      = 0,
    SHADING_MODEL_TRANSLUCENT = 1,
};

force_inline bool r_should_enable_depth(R_Shading_Model sm) {
    return sm != SHADING_MODEL_TRANSLUCENT;
}

force_inline bool r_should_enable_blend(R_Shading_Model sm) {
    return sm == SHADING_MODEL_TRANSLUCENT;
}

struct Material_Buffer {
    RHI_Buffer      buffer;
    RHI_Buffer_View view;
};

struct R_Rect {
    f32 x, y, w, h;
};

struct Renderer {
    Allocator heap;

    volatile b32 initted;

    // Ring buffer shared with game thread.
    Render_SPSC_Queue ring;

    // Textures
    Guid scene_depth[RHI_MAX_BUFFER_COUNT];
    Guid gbuffer_color[RHI_MAX_BUFFER_COUNT];
    Guid scene_texture[RHI_MAX_BUFFER_COUNT];

    // Full-screen Triangle Mesh
    f32 fullscreen_triangle_vertices[3];
    u32 fullscreen_triangle_indices[3];
    Guid fullscreen_triangle_mesh;

    // Global shader
    Guid postprocess_pipeline;
    Guid composition_pipeline;

    // Registered Passes
    Array<R_Pass*> passes;


    Material_Buffer material_buffer;
    u64 material_buffer_used = 0;


    // @Temporary
    RHI_Buffer           camera_buffer;
    RHI_Buffer_View      camera_view;
    void                *camera_ptr;
};

extern Renderer *renderer;


GPU_Camera gpu_camera_from_game(Camera *camera);

void       r_render(Game_State *g, f64 refresh_dt);
void       r_entry(void *param);


void r_pipeline_create(Guid id,
                       String shader_filepath, 
                       String material_filepath,
                       R_Shading_Model shading_model);
void r_pipeline_destroy(Guid id);


#endif // RTS_RENDERER_H
