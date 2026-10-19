// Copyright Seong Woo Lee. All Rights Reserved.

#include "./terrain.h"
#include "basic/allocator.h"
#include "gfx/gfx.h"
#include "game.h"
#include "asset/asset.h"

void
terrain_init(Game_State *g, 
             Terrain    *terrain, 
             Guid        height_map,
             f32         height_scale,
             s64         cell_count_x, 
             s64         cell_count_y,
             f32         cell_size)
{
  terrain->vertices_x   = cell_count_x + 1;
  terrain->vertices_y   = cell_count_y + 1;
  terrain->cell_size    = cell_size;

  s64 n = terrain->vertices_x * terrain->vertices_y;

  f32 *heights = (f32*)game_alloc(g, sizeof(f32)*n, align_of(f32));
  terrain->heights = handle_from_pointer(g, heights);

  // @Temporary: Generate heights
  for (s64 y = 0; y < terrain->vertices_y; ++y)
  {
    for (s64 x = 0; x < terrain->vertices_x; ++x)
    {
      s64 index = terrain->vertices_x*y + x;
      heights[index] = height_scale * 0.5f * (m_sin(x * 0.05f) * m_cos(y * 0.05f) + 1.f);
    }
  }

  
  f32 dim_x = cell_count_x * cell_size;
  f32 dim_y = cell_count_y * cell_size;

  terrain->corner_x = -0.5f * dim_x;
  terrain->corner_y = -0.5f * dim_y;

  {
    terrain->height_map_id = guid_generate();

    RHI_Texture_Desc desc = {};
    desc.name           = S("Terrain Heightmap");
    desc.type           = RHI_TEXTURE_TYPE_2D;
    desc.format         = RHI_FORMAT_R16F;
    desc.usage          = RHI_TEXTURE_USAGE_SAMPLED;
    desc.width          = terrain->vertices_x;
    desc.height         = terrain->vertices_y;
    desc.mip_levels     = 1;
    desc.depth          = 1;

    gfx_texture_create(terrain->height_map_id, desc);
  }

  // @Temporary
  terrain->material_id = guid_from_string(S("terrain/terrain.material"));
  asset_request(terrain->material_id, NULL_GUID, NULL);

  terrain->initted = true;
}

void 
terrain_deinit(Terrain *terrain)
{
  gfx_texture_destroy(terrain->height_map_id);
  memset(terrain, 0, sizeof(Terrain));
}
