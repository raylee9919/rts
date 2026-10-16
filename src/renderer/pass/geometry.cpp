// Copyright Seong Woo Lee. All Rights Reserved.geometry.cpp

#include "./geometry.h"
#include "gfx/gfx.h"
#include "game.h"
#include "material/material.h"
#include "mesh.h"
#include "animation.h"
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

  { // Create skinning buffer and view
    u64 stride = sizeof(m3x4);
    u64 sz     = stride * R_MAX_SKINNING_MATRICES;

    RHI_Buffer_Desc desc = {};
    desc.memory_type = RHI_MEMORY_UPLOAD;
    desc.size        = sz;

    R_ASSERT(rhi_buffer_init(gfx->device, &result->skinning_buffer, &desc, NULL));

    RHI_Buffer_View_Desc view_desc = {};
    {
      view_desc.type     = RHI_BUFFER_VIEW_TYPE_STRUCTURED;
      view_desc.writable = false;
      view_desc.stride   = stride;
      view_desc.offset   = 0;
      view_desc.size     = sz;
    }

    rhi_buffer_view_init(gfx->device, &result->skinning_view, &result->skinning_buffer, &view_desc);

    result->skinning_ptr = rhi_buffer_map(&result->skinning_buffer);
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
  pass->skinning_used = 0;

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
      R_Pass_Geometry *pass = data->pass;

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

              // Push constants
              R_Pass_Geometry::Push_Constants c = {};
              c.vertex_buffer_id    = gfx_mesh->vertex_buffer_view.bindless;
              c.linear_sampler_id   = gfx->linear_sampler.bindless;
              c.camera_buffer_id    = renderer->camera_view.bindless;
              c.argument_buffer_id  = pass->arguments_view.bindless;
              c.argument_base_index = data->num_draws;
              c.material_buffer_id  = renderer->material_buffer.view.bindless;
              c.skinning_buffer_id  = pass->skinning_view.bindless;
              gfx_push_constants(&c, sizeof(c));

              // Upload arguments
              auto *args = (R_Pass_Geometry::Arguments*)pass->arguments_ptr + data->num_draws;
              memset(args, 0, sizeof(*args));
              m4x4 transform = m4x4_translate(E->position) * m4x4_scale(E->scale.x, E->scale.y, E->scale.z);
              memcpy(&args->transform, &transform, sizeof(args->transform));
              args->material_index    = material->offset;


              if (mesh.skeleton != NULL_GUID)
              {
                Skeleton *skel = skeleton_from_guid(mesh.skeleton);
                R_ASSERT(skel);

                args->num_matrices      = skel->num_joints;
                args->skinning_matrices = pass->skinning_used;

                if (E->animation_id != NULL_GUID)
                {
                  Sampled_Animation *anim = animation_from_guid(E->animation_id);
                  R_ASSERT(anim);

                  f64 t = fmod(g->time, (f64)anim->duration);
                  if (t < 0) t += anim->duration;

                  f64 f = t * anim->frame_rate;
                  s64 i0 = (s64)f;
                  if (i0 > (s64)anim->samples.count - 1) 
                    i0 = anim->samples.count - 1;
                  s64 i1 = (i0 + 1 < (s64)anim->samples.count) ? i0 + 1 : i0;
                  f32 a = (f32)(f - (f64)i0);

                  Animation_Sample *sample0 = anim->samples.data + i0;
                  Animation_Sample *sample1 = anim->samples.data + i1;

                  m4x4 *local     = (m4x4*)alloc(sizeof(m4x4)*skel->num_joints, tctx.temp);
                  m4x4 *composite = (m4x4*)alloc(sizeof(m4x4)*skel->num_joints, tctx.temp);

                  for (s64 j = 0; j < skel->num_joints; ++j) 
                  {
                    Joint *joint = skel->joints + j;

                    Xform *xform0 = sample0->xforms.data + j;
                    Xform *xform1 = sample1->xforms.data + j;

                    vec3 t = lerp(xform0->translation, xform1->translation, a);
                    Quaternion r;
                    if (dot(xform0->rotation, xform1->rotation) < 0) {
                      r = nlerp(xform0->rotation, -xform1->rotation, a);
                    } else {
                      r = nlerp(xform0->rotation, xform1->rotation, a);
                    }
                    vec3 s = lerp(xform0->scale, xform1->scale, a);
                    Xform xform(t, r, s);

                    m4x4 m = m4x4_from_xform(&xform);

                    if (joint->parent < 0) {
                      local[j] = m;
                    } else {
                      local[j] = local[joint->parent] * m;
                    }
                  }

                  for (s64 j = 0; j < skel->num_joints; ++j) 
                  {
                    Joint *joint = skel->joints + j;
                    composite[j] = local[j] * joint->inverse_bind_matrix;
                    memcpy((m3x4*)pass->skinning_ptr + pass->skinning_used + j, composite + j, sizeof(m3x4));
                  }

                  pass->skinning_used += skel->num_joints;


#if 1
                  // @Temporary: Visualize skeleton
                  { 
                    GPU_Camera cam = gpu_camera_from_game(&game_state->camera);
                    m4x4 transform = m4x4_translate(E->position) * m4x4_scale(E->scale.x, E->scale.y, E->scale.z);
                    m4x4 mvp = cam.view_proj * transform;

                    vec2 *px    = (vec2*)alloc(sizeof(vec2) * skel->num_joints, tctx.temp);
                    b32  *vis   = (b32*) alloc(sizeof(b32)  * skel->num_joints, tctx.temp);

                    for (s32 i = 0; i < skel->num_joints; ++i)
                    {
                      m4x4 m = local[i];
                      vec4 clip = mvp * vec4(m._14, m._24, m._34, 1.f);
                      vis[i]    = clip.w > 0.f;
                      vec2 ndc  = clip.xy * (1.f/clip.w);
                      px[i]     = vec2(( ndc.x*0.5f + 0.5f) * (f32)RESOLUTION_X,
                                       (-ndc.y*0.5f + 0.5f) * (f32)RESOLUTION_Y);

                      if (skel->joints[i].parent >= 0 && vis[i] && vis[skel->joints[i].parent])
                        draw_line(px[skel->joints[i].parent], px[i], vec4(1,1,0,1), 1.5f);
                    }
                  }
#endif
                }
              }


              // @Temporary: Draw AABB
              if (1) 
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
