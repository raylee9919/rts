// Copyright Seong Woo Lee. All Rights Reserved.

#include "./mesh.h"
#include "os/os.h"
#include "basic/context.h"
#include "basic/log.h"
#include "basic/string_builder.h"
#include "asset_lexer.h"
#include "asset/asset.h"

#include "renderer/renderer.h"


static String
string_from_mesh(Triangle_Mesh *mesh, Allocator allocator)
{
  String s = {};

  String_Builder b = {};
  init(&b, allocator);

  append(&b, S("\n// Name\n"));
  append(&b, tprint(S("\"%S\"\n"), mesh->name));

  append(&b, S("\n// Vertices\n"));
  append(&b, tprint(S("%d\n"), mesh->num_vertices));
  for (s64 i = 0; i < mesh->num_vertices; ++i)
  {
    Vertex *v = mesh->vertices + i;
    append(&b, tprint(S("%.6f %.6f %.6f\n"), v->position[0], v->position[1], v->position[2]));
    append(&b, tprint(S("%.6f %.6f\n"), v->uv[0], v->uv[1]));
    append(&b, tprint(S("%.6f %.6f %.6f\n"), v->normal[0], v->normal[1], v->normal[2]));
    append(&b, tprint(S("%.6f %.6f %.6f %.6f\n"), v->tangent[0], v->tangent[1], v->tangent[2], v->tangent[3]));
    append(&b, S("\n"));
  }

  append(&b, S("\n// Indices\n"));
  append(&b, tprint(S("%d\n"), mesh->num_indices));
  for (s64 i = 0; i < mesh->num_indices; ++i)
  {
    u32 index = mesh->indices[i];
    append(&b, tprint(S("%u "), index));
    if (i % 8 == 7)
    {
      append(&b, S("\n"));
    }
  }

  append(&b, S("\n// Parts\n"));
  append(&b, tprint(S("%d\n"), mesh->num_parts));
  for (s64 i = 0; i < mesh->num_parts; ++i)
  {
    Mesh_Part *part = &mesh->parts[i];
    append(&b, tprint(S("%u "), part->offset));
    append(&b, tprint(S("%u "), part->count));
    append(&b, S("\n"));
  }

  s = flush(&b);

  return s;
}

static String
string_from_meshes(Array<Triangle_Mesh> meshes, Allocator allocator)
{
  String s = {};

  String_Builder b = {};
  init(&b, allocator);

  append(&b, S("\n// Triangle Meshes\n"));
  append(&b, tprint(S("%d\n"), meshes.count));
  for (u32 i = 0; i < meshes.count; ++i)
  {
    Triangle_Mesh *mesh = &meshes.data[i];
    append(&b, string_from_mesh(mesh, tctx.temp));
  }

  s = flush(&b);

  return s;
}

String
asset_string_from_meshes(Array<Triangle_Mesh> meshes, 
                         Allocator allocator)
{
  String s = {};

  String_Builder b = {};
  init(&b, allocator);

  append(&b, S("// Version\n"));
  append(&b, S("1\n"));
  append(&b, string_from_meshes(meshes, tctx.temp));

  s = flush(&b);

  return s;
}

Array<Triangle_Mesh>
parse_and_eval_mesh_asset(Asset_Lexer *lexer, 
                          Allocator    allocator)
{
  Array<Triangle_Mesh> result = {};
  result.allocator = allocator;

  s64 version = eat_unsigned_int(lexer);

  s64 num_triangle_meshes = eat_unsigned_int(lexer);
  array_reserve(&result, num_triangle_meshes);

  for (int mesh_index = 0;
       mesh_index < num_triangle_meshes;
       mesh_index += 1)
  {
    Triangle_Mesh tm = {};

    String name = eat_string(lexer);
    tm.name = copy_string(name, allocator);

    // Vertices
    tm.num_vertices = eat_unsigned_int(lexer);
    tm.vertices = (Vertex*)alloc(sizeof(Vertex)*tm.num_vertices, allocator);
    for (int v = 0; v < tm.num_vertices; ++v)
    {
      Vertex* vertex = tm.vertices + v;
      vertex->position[0] = eat_signed_float(lexer);
      vertex->position[1] = eat_signed_float(lexer);
      vertex->position[2] = eat_signed_float(lexer);

      vertex->uv[0] = eat_signed_float(lexer);
      vertex->uv[1] = eat_signed_float(lexer);

      vertex->normal[0] = eat_signed_float(lexer);
      vertex->normal[1] = eat_signed_float(lexer);
      vertex->normal[2] = eat_signed_float(lexer);

      vertex->tangent[0] = eat_signed_float(lexer);
      vertex->tangent[1] = eat_signed_float(lexer);
      vertex->tangent[2] = eat_signed_float(lexer);
      vertex->tangent[3] = eat_signed_float(lexer);
    }

    // Indices
    tm.num_indices = eat_unsigned_int(lexer);
    tm.indices = (u32*)alloc(sizeof(u32)*tm.num_indices, allocator);
    for (int j = 0; j < tm.num_indices; ++j)
    {
      tm.indices[j] = eat_unsigned_int(lexer);
    }

    // Parts
    tm.num_parts = eat_unsigned_int(lexer);
    tm.parts = (Mesh_Part*)alloc(sizeof(Mesh_Part)*tm.num_parts, allocator);
    for (int p = 0; p < tm.num_parts; ++p)
    {
      tm.parts[p].offset = eat_unsigned_int(lexer);
      tm.parts[p].count  = eat_unsigned_int(lexer);
    }

    array_add(&result, tm);
  }

  R_ASSERT(peek_token(lexer)->type == ASSET_TOKEN_END_OF_INPUT);

  return result;
}

ASSET_LOAD_PROC(mesh_load_proc)
{
  Allocator allocator = asset_system->heap;
  Guid guid = guid_from_string(filepath);

  Asset_Lexer lexer = {};
  String asset = read_entire_file(filepath, tctx.temp, false);
  asset_tokenize(&lexer, asset, tctx.temp);
  Array<Triangle_Mesh> meshes = parse_and_eval_mesh_asset(&lexer, allocator);

  Array<Guid> guids;
  guids.allocator = allocator;

  r_meshes_alloc(guid, meshes);
}
