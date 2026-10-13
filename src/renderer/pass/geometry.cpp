// Copyright Seong Woo Lee. All Rights Reserved.

#include "./geometry.h"
#include "gfx/gfx.h"
#include "game.h"
#include "material/material.h"
#include "animation/animation.h"
#include "math/math.h"

#define R_MAX_SKINNING_MATRICES 65536 // @Temporary

R_PASS_INIT( RenderPassInit_Geometry )
{
  auto *result = (R_Pass_Geometry *)alloc(sizeof(R_Pass_Geometry), renderer->heap);
  Construct( result );

  result->name    = S("Geometry");
  result->deinit  = RenderPassDeinit_Geometry;
  result->execute = RenderPassExecute_Geometry;

  { // Create arguments buffer and view
    u64 stride = sizeof(R_Pass_Geometry::Arguments);
    u64 sz     = stride * 16777216; // @Temporary

    RHI_Buffer_Desc desc = {};
    desc.memory_type = RHI_MEMORY_UPLOAD;
    desc.size        = sz;

    R_ASSERT(rhi_buffer_init(gfx->device, &result->arguments_buffer, &desc, NULL));

    RHI_Buffer_View_Desc view_desc = {};
    {
      view_desc.type     = RHI_BUFFER_VIEW_TYPE_STRUCTURED;
      view_desc.writable = false;
      view_desc.stride   = stride;
      view_desc.offset   = 0;
      view_desc.size     = sz;
    }

    rhi_buffer_view_init(gfx->device, &result->arguments_view, &result->arguments_buffer, &view_desc);

    result->arguments_ptr = rhi_buffer_map(&result->arguments_buffer);
  }

  return result;
}

R_PASS_DEINIT( RenderPassDeinit_Geometry )
{
  dealloc(inPass, renderer->heap);
}

R_PASS_EXECUTE( RenderPassExecute_Geometry )
{
  auto *pass = (R_Pass_Geometry *)inPass;

  u32 res_x = inInfo.resolution_x;
  u32 res_y = inInfo.resolution_y;

  Game_State *g = inInfo.game_state;

  GFX_Pass gfx_pass  = {};
  gfx_pass.name                 = inPass->name;
  gfx_pass.viewport             = {0.f, 0.f, (f32)res_x, (f32)res_y};
  gfx_pass.scissor              = {0, 0, res_x, res_y};
  gfx_pass.color_attachments[0] = renderer->gbuffer_color[gfx_backbuffer_index()];
  gfx_pass.depth_attachment     = renderer->scene_depth[gfx_backbuffer_index()];
  gfx_pass.min_depth            = 0.f;
  gfx_pass.max_depth            = 1.f;

  // Upload camera
  GPU_Camera gpu_camera = gpu_camera_from_game(&g->camera);
  memcpy(renderer->camera_ptr, &gpu_camera, sizeof(gpu_camera));

  struct Data {
    R_Pass_Geometry *pass;
    GPU_Camera *camera;
    s64 num_draws;
  } data_;
  data_.pass = pass;
  data_.num_draws = 0;
  data_.camera = &gpu_camera;

  gfx_pass_begin(R_PASS_GEOMETRY, &gfx_pass);
  {
    entity_dfs(g, g->root, &data_, [](Game_State *g, Entity *E, u64 counter, void *data_) {
      Data *data = (Data*)data_;

      // One mesh asset may contain multiple meshes.
      Array<R_Mesh> *meshes = table_find_pointer(&renderer->mesh_table, E->mesh);
      if (meshes)
      {
        for (R_Mesh mesh : *meshes)
        {
          // Low-level mesh for binding vertex and index buffers.
          GFX_Mesh *gfx_mesh = table_find_pointer(&gfx->mesh_table, mesh.id);
          if (gfx_mesh)
          {
            // Draw each part with its own material.
            u32 num_parts = mesh.offsets.count;
            for (u32 part_idx = 0; part_idx < num_parts; ++part_idx)
            {
              // Indices
              u32 index_offset = mesh.offsets[part_idx];
              u32 index_count  = mesh.counts[part_idx];

              // Set pipeline
              Guid material_id = mesh.materials[part_idx];
              M_Entry *material = material_from_guid(material_id);
              gfx_set_pipeline(material->pipeline);

              // Upload arguments
              R_Pass_Geometry *pass = data->pass;
              auto *args = (R_Pass_Geometry::Arguments*)pass->arguments_ptr + data->num_draws;
              m4x4 transform = m4x4_translate(E->position) * m4x4_scale(E->scale.x, E->scale.y, E->scale.z);
              memcpy(&args->transform, &transform, sizeof(args->transform));
              args->material_index = material->offset;

              // Push constants
              R_Pass_Geometry::Push_Constants c = {};
              c.vertex_buffer_id    = gfx_mesh->vertex_buffer_view.bindless;
              c.linear_sampler_id   = gfx->linear_sampler.bindless;
              c.camera_buffer_id    = renderer->camera_view.bindless;
              c.argument_buffer_id  = pass->arguments_view.bindless;
              c.argument_base_index = data->num_draws;
              c.material_buffer_id  = renderer->material_buffer.view.bindless;
              gfx_push_constants(&c, sizeof(c));

              // @Temporary: Draw AABB
              {
                m4x4 mvp = data->camera->view_proj * transform;
                vec3 lo = mesh.aabb.min;
                vec3 hi = mesh.aabb.max;

                // Corner i: bit 0 -> x, bit 1 -> y, bit 2 -> z (0 = min, 1 = max)
                vec2 px[8];
                b32  visible[8];
                for (int i = 0; i < 8; ++i)
                {
                  vec3 c = vec3((i & 1) ? hi.x : lo.x,
                                (i & 2) ? hi.y : lo.y,
                                (i & 4) ? hi.z : lo.z);
                  vec4 clip = mvp * vec4(c, 1.f);
                  visible[i] = clip.w > 0.f;
                  vec2 ndc = clip.xy * (1.f/clip.w);
                  px[i] = vec2((ndc.x*0.5f + 0.5f) * 2560, (0.5f - ndc.y*0.5f) * 1440);
                }

                // 12 edges: connect corners whose indices differ in exactly one bit
                for (int i = 0; i < 8; ++i)
                {
                  for (int bit = 1; bit < 8; bit <<= 1)
                  {
                    if (i & bit) continue;
                    int j = i | bit;
                    if (visible[i] && visible[j])
                      draw_line(px[i], px[j], vec4(1, 0, 1, 1), 2.f);
                  }
                }
              }

              // Draw
              gfx_draw(mesh.id, 1, index_offset, index_count);
              data->num_draws += 1;
            }
          }
        }
      }
    });
  }
  gfx_pass_end();
}
