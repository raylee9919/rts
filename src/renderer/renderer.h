// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RTS_RENDERER_H
#define RTS_RENDERER_H

#include "basic/core.h"
#include "os/os.h"
#include "rhi/rhi.h"
#include "gfx/gfx.h"
#include "shared/shared.h"

#define R_DEPTH_FORMAT  RHI_FORMAT_D32F
#define R_COLOR_FORMAT  RHI_FORMAT_RGBA16F

struct Game_State;
struct Camera;

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
    return sm == SHADING_MODEL_TRANSLUCENT;
}

force_inline bool r_should_enable_blend(R_Shading_Model sm) {
    return sm == SHADING_MODEL_TRANSLUCENT;
}

struct Material {
    R_Shading_Model shading_model = {};

    vec3    albedo         = vec3{1.f, 1.f, 1.f};
    f32     metallic       = 0.f;
    f32     roughness      = 1.f;

    Guid    albedo_texture = NULL_GUID;
    Guid    orm_texture    = NULL_GUID;

    Guid    pipeline       = NULL_GUID;
};

struct Renderer {
    Arena *arena;
    Allocator heap;

    volatile b32 initted;

    Guid scene_depth[RHI_MAX_BUFFER_COUNT];
    Guid gbuffer_color[RHI_MAX_BUFFER_COUNT];
    Guid scene[RHI_MAX_BUFFER_COUNT];

    Table<Guid, Material, gfx_128_to_32> material_table;
};

extern Renderer *renderer;
extern Render_SPSC_Queue render_queue;


extern Guid                 cube_mesh;
extern RHI_Buffer           arguments_buffer;
extern RHI_Buffer_View      arguments_view;
extern void                *arguments_ptr;
extern RHI_Buffer           material_buffer;
extern RHI_Buffer_View      material_view;
extern void                *material_ptr;
extern RHI_Buffer           camera_buffer;
extern RHI_Buffer_View      camera_view;
extern void                *camera_ptr;

GPU_Camera gpu_camera_from_game(Camera *camera);

void       r_render(Game_State *g, f64 refresh_dt);
void       r_entry(void *param);


Material     *r_material_alloc(Guid guid);
void          r_material_dealloc(Guid guid);
Material     *r_material_from_guid(Guid guid);
GPU_Material  to_gpu_material(Material *material);



void R_pipeline_create(Guid id,
                       String shader_filepath, 
                       R_Shading_Model shading_model);
void R_pipeline_destroy(Guid id);

void R_ring_init();
void R_ring_deinit();

#endif
