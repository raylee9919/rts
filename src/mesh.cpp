// Copyright Seong Woo Lee. All Rights Reserved.

#include "./mesh.h"
#include "os/os.h"
#include "basic/context.h"
#include "basic/string_builder.h"
#include "asset/asset.h"
#include "asset/lexer.h"

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

  append(&b, S("\n// AABB\n"));
  append(&b, tprint(S("%.6f "), mesh->aabb.min.x));
  append(&b, tprint(S("%.6f "), mesh->aabb.min.y));
  append(&b, tprint(S("%.6f\n"), mesh->aabb.min.z));
  append(&b, tprint(S("%.6f "), mesh->aabb.max.x));
  append(&b, tprint(S("%.6f "), mesh->aabb.max.y));
  append(&b, tprint(S("%.6f\n"), mesh->aabb.max.z));

  append(&b, S("\n// Parts\n"));
  append(&b, tprint(S("%d\n"), mesh->num_parts));
  for (s64 i = 0; i < mesh->num_parts; ++i)
  {
    Mesh_Part *part = &mesh->parts[i];
    append(&b, tprint(S("%u "), part->offset));
    append(&b, tprint(S("%u "), part->count));
    append(&b, tprint(S("@\"%S\" "), part->material_path));
    append(&b, S("\n"));
  }

  s = flush(&b);

  return s;
}

String
asset_string_from_skeleton(Skeleton *sk, Allocator allocator)
{
  String s = {};

  String_Builder b = {};
  init(&b, allocator);

  append(&b, S("1 // version\n\n"));

  append(&b, tprint(S("%d\n\n"), sk->num_joints));
  for (s64 i = 0; i < sk->num_joints; ++i)
  {
    Joint *j = &sk->joints[i];
    append(&b, tprint(S("\"%S\"\n"), j->name));

    append(&b, tprint(S("%d\n"), j->parent));

    m4x4 l = j->local_transform;
    append(&b, tprint(S("%.6f %.6f %.6f %.6f\n"), l._11, l._12, l._13, l._14));
    append(&b, tprint(S("%.6f %.6f %.6f %.6f\n"), l._21, l._22, l._23, l._24));
    append(&b, tprint(S("%.6f %.6f %.6f %.6f\n"), l._31, l._32, l._33, l._34));
    append(&b, tprint(S("%.6f %.6f %.6f %.6f\n"), l._41, l._42, l._43, l._44));

    m4x4 inv = j->inverse_bind_matrix;
    append(&b, tprint(S("%.6f %.6f %.6f %.6f\n"), inv._11, inv._12, inv._13, inv._14));
    append(&b, tprint(S("%.6f %.6f %.6f %.6f\n"), inv._21, inv._22, inv._23, inv._24));
    append(&b, tprint(S("%.6f %.6f %.6f %.6f\n"), inv._31, inv._32, inv._33, inv._34));
    append(&b, tprint(S("%.6f %.6f %.6f %.6f\n"), inv._41, inv._42, inv._43, inv._44));

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

static vec2
_eat_vec2(Asset_Lexer *lexer)
{
  vec2 v;
  v.x = eat_signed_float(lexer);
  v.y = eat_signed_float(lexer);
  return v;
}

static vec3
_eat_vec3(Asset_Lexer *lexer)
{
  vec3 v;
  v.x = eat_signed_float(lexer);
  v.y = eat_signed_float(lexer);
  v.z = eat_signed_float(lexer);
  return v;
}

Array<Triangle_Mesh>
parse_and_eval_mesh_asset(Asset_Lexer *lexer, 
                          Allocator    allocator)
{
  Array<Triangle_Mesh> result = {};
  result.allocator = allocator;

  s64 version = eat_unsigned_int(lexer);
  R_ASSERT(version == 1);

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

    // AABB
    tm.aabb.min = _eat_vec3(lexer);
    tm.aabb.max = _eat_vec3(lexer);

    // Parts
    tm.num_parts = eat_unsigned_int(lexer);
    tm.parts = (Mesh_Part*)alloc(sizeof(Mesh_Part)*tm.num_parts, allocator);
    for (int p = 0; p < tm.num_parts; ++p)
    {
      tm.parts[p].offset = eat_unsigned_int(lexer);
      tm.parts[p].count  = eat_unsigned_int(lexer);


      // @Temporary: request material
      Guid ref = eat_reference(lexer);
      asset_request(ref, lexer->id);
      tm.parts[p].material_path = asset_name_from_guid(ref);
    }

    append(&result, tm);
  }

  R_ASSERT(peek_token(lexer)->type == ASSET_TOKEN_END_OF_INPUT);

  return result;
}

ASSET_LOAD_PROC(mesh_load_proc)
{
  Asset_Lexer lexer = {};
  asset_tokenize(&lexer, id, tctx.temp);

  Allocator allocator = asset_system->heap;

  Array<Triangle_Mesh> meshes = parse_and_eval_mesh_asset(&lexer, allocator);

  Array<Guid> guids;
  guids.allocator = allocator;

  r_meshes_alloc(id, meshes);
}

static Skeleton
parse_and_eval_skeleton_asset(Asset_Lexer *lexer, Allocator allocator)
{
  Skeleton result = {};

  s64 version = eat_unsigned_int(lexer);
  R_ASSERT(version == 1);

  result.num_joints = eat_unsigned_int(lexer);
  result.joints = (Joint*)alloc(sizeof(Joint) * result.num_joints, allocator);

  for (s64 i = 0; i < result.num_joints; i += 1)
  {
    Joint *j = result.joints + i;
    j->name                = eat_string(lexer);
    j->parent              = eat_signed_int(lexer);
    j->local_transform     = eat_m4x4(lexer);
    j->inverse_bind_matrix = eat_m4x4(lexer);
  }

  R_ASSERT(peek_token(lexer)->type == ASSET_TOKEN_END_OF_INPUT);

  return result;
}

ASSET_LOAD_PROC(skeleton_load_proc)
{
  Asset_Lexer lexer = {};
  asset_tokenize(&lexer, id, tctx.temp);

  Allocator allocator = asset_system->heap;

  Skeleton skeleton = parse_and_eval_skeleton_asset(&lexer, allocator);
}
