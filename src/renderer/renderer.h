// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RTS_RENDERER_H
#define RTS_RENDERER_H

#include "basic/core.h"
#include "os/os.h"
#include "rhi/rhi.h"
#include "gfx/gfx.h"
#include "renderer/immediate.h"
#include "shaders/shared/shared.h"
#include "mesh.h"

#define R_DEPTH_FORMAT  RHI_FORMAT_D32F
#define R_COLOR_FORMAT  RHI_FORMAT_RGBA16F

// @Temporary
#define RESOLUTION_X 2560
#define RESOLUTION_Y 1440


struct Game_State;
struct Camera;
struct R_Pass;


// Built from `Triangle_Mesh`, consists of 
// a mesh and slices of material.
struct R_Mesh
{
  Guid          id;
  Guid          skeleton;
  AABB          aabb;
  Array<Guid>   materials;
  Array<u32>    offsets;
  Array<u32>    counts;
};


struct R_Pass_Execute_Info 
{
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

enum Render_Pass : u16 
{
  R_PASS_GEOMETRY = 0,
  R_PASS_POSTPROCESS,
  R_PASS_UI,
  R_PASS_COMPOSITION,
};

struct Render_Entry 
{
  Mutex           mutex;
  Game_State     *game_state;

  R_Quad          quads[R_MAX_QUADS];
  u32             num_quads;
};

struct Render_Ring 
{
  Mutex           mutex;
  Condvar         condvar;

  Render_Entry    entries[3]; // cap = 2
  s32             read_idx   = 0;
  s32             write_idx  = 0;


  b32 is_empty() {
    return read_idx == write_idx;
  }

  b32 is_full() {
    return (write_idx + 1) % array_count(entries) == read_idx;
  }
};


// @Cleanup: Not fan of those...
enum R_Shading_Model {
  SHADING_MODEL_OPAQUE      = 0,
  SHADING_MODEL_TRANSLUCENT = 1,
};

force_inline bool 
r_should_enable_depth(R_Shading_Model sm) 
{
  return sm != SHADING_MODEL_TRANSLUCENT;
}

force_inline bool 
r_should_enable_blend(R_Shading_Model sm) 
{
  return sm == SHADING_MODEL_TRANSLUCENT;
}

struct Material_Buffer {
  RHI_Buffer      buffer;
  RHI_Buffer_View view;
};

struct R_Rect {
  f32 x, y, w, h;
};

struct Renderer 
{
  Allocator heap;

  volatile b32 initted;

  // Ring buffer shared with game thread.
  Render_Ring ring;

  // Render Targets
  Guid scene_depth;
  Guid gbuffer_color;
  Guid scene_texture;
  Guid ui_texture;

  // Full-screen Triangle Mesh
  f32  fullscreen_triangle_vertices[3];
  u32  fullscreen_triangle_indices[3];
  Guid fullscreen_triangle_mesh;

  // Global shader
  Guid postprocess_pipeline;
  Guid composition_pipeline;

  // Registered Passes
  Array<R_Pass*> passes;

  // @Temporary
  Material_Buffer material_buffer;
  u64 material_buffer_used = 0;

  // Mesh table
  Table<Guid, Array<R_Mesh>, hash_guid> mesh_table;
};

extern Renderer *renderer;


GPU_Camera 
gpu_camera_from_game(Camera *camera);

void 
r_entry(void *param);

void 
r_pass_create(R_Pass_Init_Proc *init_proc);

void 
r_pass_destroy();

void 
r_pipeline_create(Guid            id,
                  String          shader_filepath, 
                  String          vertex_shader_entry,
                  String          material_filepath,
                  R_Shading_Model shading_model);
void 
r_pipeline_destroy(Guid id);


vec2 
playfield_from_window(s64 x, s64 y);


void
r_meshes_alloc(Guid id, 
               Array<Triangle_Mesh> meshes);

#endif // RTS_RENDERER_H
