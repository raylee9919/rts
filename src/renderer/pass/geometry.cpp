// Copyright Seong Woo Lee. All Rights Reserved.geometry.cpp

#include "./geometry.h"
#include "gfx/gfx.h"
#include "game.h"
#include "material/material.h"
#include "mesh.h"
#include "animation.h"
#include "math/math.h"
#include "camera.h"

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

  gfx_pass_begin(R_PASS_GEOMETRY, &gfx_pass);
  {
    //
    // Draw terrain
    //
    {
      Terrain *terrain = &g->terrain;
      R_ASSERT(terrain->initted);

      // @Fix: This won't work in loaded game state...
      if (!pass->terrain_initted)
      {
        // Create pipeline
        pass->terrain_pipeline = guid_generate();
        r_pipeline_create(pass->terrain_pipeline,
                          S("shaders/pass/terrain.slang"),
                          S("main_vs"),
                          S("shaders/material/default.slang"),
                          SHADING_MODEL_OPAQUE);


        // Create mesh
        pass->terrain_mesh_id = guid_generate();
        struct Terrain_Vertex {
          float position[3];
          float uv[2];
          float normal[3];
          float tangent[4];
        };

        s64 cell_x       = terrain->vertices_x - 1;
        s64 cell_y       = terrain->vertices_y - 1;
        f32 cell_size    = terrain->cell_size;
        s64 num_vertices = terrain->vertices_x * terrain->vertices_y;
        s64 num_indices  = cell_x*cell_y*6;
        f32 *heights     = (f32*)pointer_from_handle(g, terrain->heights);

        Terrain_Vertex *vertices = (Terrain_Vertex*)alloc(sizeof(Terrain_Vertex)*num_vertices, tctx.temp);
        u32 *indices = (u32*)alloc(sizeof(u32)*num_indices, tctx.temp);

        // Make vertices
        for (s64 y = 0; y < terrain->vertices_y; ++y)
        {
          for (s64 x = 0; x < terrain->vertices_x; ++x)
          {
            s64 index = y*terrain->vertices_x + x;
            f32 height = heights[index];

            Terrain_Vertex *vert = vertices + index;

            vert->position[0] = terrain->corner_x + x*cell_size;
            vert->position[1] = terrain->corner_y + y*cell_size;
            vert->position[2] = height;
            
            vert->uv[0] = (f32)x / (f32)terrain->vertices_x;
            vert->uv[1] = (f32)y / (f32)terrain->vertices_y;

            s64 xl = max(x - 1, (s64)0);
            s64 xr = min(x + 1, terrain->vertices_x - 1);
            s64 yd = max(y - 1, (s64)0);
            s64 yu = min(y + 1, terrain->vertices_y - 1);

            f32 dhdx = (heights[y*terrain->vertices_x + xr] - heights[y*terrain->vertices_x + xl]) / ((f32)(xr - xl) * cell_size);
            f32 dhdy = (heights[yu*terrain->vertices_x + x] - heights[yd*terrain->vertices_x + x]) / ((f32)(yu - yd) * cell_size);

            vec3 n = normalize(vec3(-dhdx, -dhdy, 1.f));
            vert->normal[0] = n.x;
            vert->normal[1] = n.y;
            vert->normal[2] = n.z;

            vec3 t = normalize(vec3(1.f, 0.f, dhdx));
            vert->tangent[0] = t.x;
            vert->tangent[1] = t.y;
            vert->tangent[2] = t.z;
            vert->tangent[3] = 1.f;
          }
        }

        // Make indices
        u32 *dst = indices;
        for (s64 y = 0; y < cell_y; ++y)
        {
          for (s64 x = 0; x < cell_x; ++x)
          {
            u32 i00 = (u32)(y*terrain->vertices_x + x);
            u32 i10 = i00 + 1;
            u32 i01 = i00 + terrain->vertices_x;
            u32 i11 = i01 + 1;

            if (m_abs(heights[i11] - heights[i00]) <= m_abs(heights[i10] - heights[i01])) {
              // Diagonal is '00-11'
              *dst++ = i00;
              *dst++ = i11;
              *dst++ = i01;
              *dst++ = i00;
              *dst++ = i10;
              *dst++ = i11;
            } else {
              // Diagonal is '10-01'
              *dst++ = i00;
              *dst++ = i10;
              *dst++ = i01;
              *dst++ = i10;
              *dst++ = i11;
              *dst++ = i01;
            }
          }
        }

        // Make normals

        gfx_mesh_create(pass->terrain_mesh_id,
                        vertices, num_vertices, sizeof(Terrain_Vertex), 
                        indices, num_indices, sizeof(u32));


        pass->terrain_initted = true;
      }

      gfx_set_pipeline(pass->terrain_pipeline);

      GFX_Mesh *terrain_mesh = table_find_pointer(&gfx->mesh_table, pass->terrain_mesh_id);
      R_ASSERT(terrain_mesh);

      R_Pass_Geometry::Terrain_Push_Constants c = {};
      c.vertex_buffer_id       = terrain_mesh->vertex_buffer_view.bindless;
      c.linear_sampler_id      = gfx->linear_sampler.bindless;
      c.anisotropic_sampler_id = gfx->anisotropic_sampler.bindless;
      c.camera_buffer_id       = renderer->camera_view.bindless;
      c.material_buffer_id     = renderer->material_buffer.view.bindless;
      c.material_offset        = material_from_guid(terrain->material_id)->offset;
      gfx_push_constants(&c, sizeof(c));


      u32 num_indices = 6*(terrain->vertices_x-1)*(terrain->vertices_y-1);
      gfx_draw(pass->terrain_mesh_id, 1, 0, num_indices);
    }


    //
    // Draw entities
    //
    struct Data {
      R_Pass_Geometry *pass;
      GPU_Camera *camera;
      s64 num_draws;
    } data_;
    data_.pass = pass;
    data_.num_draws = 0;
    data_.camera = &gpu_camera;

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
              c.vertex_buffer_id       = gfx_mesh->vertex_buffer_view.bindless;
              c.linear_sampler_id      = gfx->linear_sampler.bindless;
              c.anisotropic_sampler_id = gfx->anisotropic_sampler.bindless;
              c.camera_buffer_id       = renderer->camera_view.bindless;
              c.argument_buffer_id     = pass->arguments_view.bindless;
              c.argument_base_index    = data->num_draws;
              c.material_buffer_id     = renderer->material_buffer.view.bindless;
              c.skinning_buffer_id     = pass->skinning_view.bindless;
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
                    Quaternion r = nlerp_shortest(xform0->rotation, xform1->rotation, a);
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
                }
              }

              // @Temporary
              if (1) 
              {
                m4x4 mvp = data->camera->view_proj * transform;
                vec3 lo = *((vec3*)&E->tight_box.lo);
                vec3 hi = *((vec3*)&E->tight_box.hi);

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
                      draw_line(px[i], px[j], vec4(0.2f, 0.2f, 0.2f, 1.f), 1.f);
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
