// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RTS_RENDERER_H
#define RTS_RENDERER_H

#include "basic/core.h"
#include "os/os.h"
#include "rhi/rhi.h"
#include "shaders/shared.h"

struct Game_State;
struct Camera;

enum Render_Pass : u32 {
    R_PASS_GEOMETRY    = 0,
    R_PASS_POSTPROCESS = 1,
    R_PASS_COMPOSITION = 2,
};

struct Render_Entry {
    Mutex           mutex;
    Game_State      *game_state;
};

struct Render_SPSC_Queue {
    Mutex           mutex;
    Condvar         condvar;

    Render_Entry    entries[3]; // cap = 2
    s32             read_idx  = 0;
    s32             write_idx = 0;


    b32 is_empty() {
        return read_idx == write_idx;
    }

    b32 is_full() {
        return (write_idx + 1) % array_count(entries) == read_idx;
    }
};

struct Renderer {
    Arena *arena;

    Guid scene_depth[RHI_MAX_BUFFER_COUNT];
    Guid gbuffer_color[RHI_MAX_BUFFER_COUNT];
    Guid scene[RHI_MAX_BUFFER_COUNT];
};

extern Render_SPSC_Queue render_queue;


extern Guid                 pipeline;
extern Guid                 cube_mesh;
extern RHI_Buffer           arguments_buffer;
extern RHI_Buffer_View      arguments_view;
extern void                *arguments_ptr;
extern RHI_Buffer           material_buffer;
extern RHI_Buffer_View      material_view;
extern void                *material_ptr;
extern Guid                 doggo_guid;
extern RHI_Buffer           camera_buffer;
extern RHI_Buffer_View      camera_view;
extern void                *camera_ptr;


GPU_Camera gpu_camera_from_game(Camera *camera);

void       r_render(Game_State *g, f64 refresh_dt);
void       r_entry(void *param);


#endif
