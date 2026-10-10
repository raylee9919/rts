// Copyright Seong Woo Lee. All Rights Reserved.

#include "./fbx.h"
#include "os/os.h"
#include "basic/log.h"
#include "basic/queue.h"
#include "basic/string_builder.h"
#include "shared.h"
#include "math/math.h"
#include "mesh.h"
#include "material/material.h"
#include "third_party/ufbx/ufbx.h"
#include "third_party/meshoptimizer/meshoptimizer.h"

/*

                      ufbx_node  ufbx_mesh  ufbx_material

  .. have 'ufbx_element' as a header. Users can access it, so they kind of
  "inherit". 'ufbx_element' is equivalent to what FBX calls as 'object' which
  is an overloaded term.

  FBX's 'Property' is key:value relationship exposed as ufbx_element.props
  You don't typically access properties directly.


                                    Node

  Like any other scenes, 'ufbx_scene' consists of 'ufbx_node's.
  Talking about hierarchy, it has 'local_transform', 'parent', 'children',
  'node_to_parent', and 'node_to_world'.

  Nodes are specialized by 'attributes'. As a matter of fact, ufbx_mesh is
  an attribute.

  Nodes and attributes can reference in either way. For example, ufbx_mesh has
  array of nodes that use it.


                                    Mesh
                                   / | \
                             Vertex Index Face
                                         / | \
                                     tri quad N-gons

  ufbx_mesh contains ufbx_vertex_attrib. List of attributes follow:

    position  <--  only this isn't optional
    normal
    tangent
    bitangent
    uv
    color
    crease

  As you know, attributes may have multiple values within a single vertex.
  So a "proper" way to identify attribute is by using index.
  It is well-documented in the header file, above the ufbx_mesh struct.

  As mesh can be instanced with different material in the scene,
  you want to look into ufbx_node instead of ufbx_material to extract materials.


                                  Animation

  'ufbx_anim_stack' is equivalent to animation clip and contains list of
  'ufbx_anim_layer'.

  Whatever. 'ufbx_bake_anim' transforms animation into simple linearly
  interpolated tracks. Iterate over stacks (clips) in the scene.


                                   Deformer
                                   /  |  \
                               skin blend cache

  We'll just focus on skin deformers.
  Skeleton = hierarchy of ufbx_nodes with ufbx_bone attribute.
  Bone influences are defined in clusters (ufbx_skin_cluster).
  ufbx_skin_cluster.vertices[] and weights[] contain per-vertex weight, and
  weights are sorted in descending order :)


                                  Dependency

  Skeleton doesn't have dependency.
  Mesh references skeleton.
  Animation references skeleton.

  Thus, process skeleton first.


                               Finding Skeletons

  I want skeletons. Starting from scene->skin_deformers isn't the way.
  Skin deformer represents relationship between a mesh and *some* bones.
  That is, even though a skeleton consists of bone A, B and C, a mesh might
  be referencing only A, so if you want a unique hierarchy of bones, it's
  better off to start from root node and find for node with bone attribute and
  regard it as a root bone.

  Or, as ufbx_bone has 'is_root' field, which means the node is a root *bone* 
  if the field is true, iterating over 'scene->bones' to find root bones is 
  feasible.

*/


static m4x4
m4x4_from_ufbx_matrix(ufbx_matrix& m)
{
  m4x4 result;
  result._11 = m.m00;
  result._12 = m.m01;
  result._13 = m.m02;

  result._21 = m.m10;
  result._22 = m.m11;
  result._23 = m.m12;

  result._31 = m.m20;
  result._32 = m.m21;
  result._33 = m.m22;

  result._14 = m.m03;
  result._24 = m.m13;
  result._34 = m.m23;

  result._41 = 0.f;
  result._42 = 0.f;
  result._43 = 0.f;
  result._44 = 1.f;
  return result;
}

static vec2
vec2_from_ufbx(ufbx_vec2 v)
{
  return vec2((f32)v.x, (f32)v.y);
}

static vec3
vec3_from_ufbx(ufbx_vec3 v)
{
  return vec3((f32)v.x, (f32)v.y, (f32)v.z);
}

static u32
_util_hash_pointer(ufbx_material *ptr)
{
  u64 u = (uintptr_t)ptr;
  return knuth_hash(u);
}

static String
_to_string(ufbx_string c)
{
  String s = { (u8*)c.data, (s64)c.length };
  return s;
}

// @Incomplete:
// Import all materials in the scene.
// At the moment, we are not doing the followings:
//
//     - prune unreferenced materials from meshes
//     - check for duplicate material name
//     - check if the texture file exists
//
static Table<ufbx_material*, String, _util_hash_pointer>
fbx_import_materials(ufbx_scene *scene)
{
  Table<ufbx_material*, String, _util_hash_pointer> result = {};
  result.allocator = tctx.temp;
 
  for (ufbx_material *material : scene->materials)
  {
    String_Builder sb = {};
    init(&sb, tctx.temp);
    {
      append(&sb, S("[1]\n\n"));

      append(&sb, tprint(S("@\"%S\"\n\n"), DEFAULT_MATERIAL_SHADER_PATH));

      struct Type_String {
        ufbx_material_pbr_map type;
        String string;
      } type_strings[] = {
        { UFBX_MATERIAL_PBR_BASE_COLOR, S("albedo_id") },
        { UFBX_MATERIAL_PBR_NORMAL_MAP, S("normal_id") },
        { UFBX_MATERIAL_PBR_ROUGHNESS,  S("roughness_id") },
        { UFBX_MATERIAL_PBR_METALNESS,  S("metallic_id") },
      };

      for (int i = 0; i < array_count(type_strings); ++i)
      {
        ufbx_texture *texture = material->pbr.maps[type_strings[i].type].texture;
        if (texture && texture->type == UFBX_TEXTURE_FILE) 
        {
          String texture_file = _to_string(texture->filename);
          if (!file_exists(texture_file))
          {
            log_warning(S("Texture: %`%S` doesn't exit. Ignoring for now..."), texture_file);
          }
          append(&sb, tprint(S("%S : \"%S\"\n"), type_strings[i].string, texture_file));
        }
      }
    }

    String s_ = flush(&sb);

    String path = tprint(S("%s.material"), material->name.data);
    File file = file_open(path, true);
    if (file_is_valid(file))
    {
      if (file_write(file, s_))
      {
        table_add(&result, material, path);
        log_info(S("Imported material to file: '%S'"), path);
      }
      else
      {
        log_error(S("Failed to write material to file: '%S'."), path);
      }
      file_close(&file);
    }
    else
    {
      log_error(S("Failed to open file: '%S' to write material."), path);
    }
  }

  return result;
}

static void
fbx_import_mesh_internal(ufbx_node*                                        node, 
                         FBX_Import_Options                                options, 
                         Array<Triangle_Mesh>*                             out_meshes,
                         Table<ufbx_material*, String, _util_hash_pointer> material_pointer_to_path,
                         Allocator                                         allocator)
{
  ufbx_mesh *mesh = node->mesh;
  if (mesh && 
      node->attrib_type == UFBX_ELEMENT_MESH && 
      node->visible)
  {
    Triangle_Mesh m = {};

    // Check if attributes exist. If it doesn't, we'll create manually.
    b32 has_normal    = mesh->vertex_normal.exists;
    b32 has_uv        = mesh->vertex_uv.exists;


    // Number of triangles if the mesh is fully triangulated.
    s64 num_tri = mesh->num_triangles;
    s64 num_indices = num_tri * 3;


    // Reserve vertices upfront. Will be deduplicated afterwards.
    Vertex *vertices = (Vertex*)alloc(sizeof(Vertex)*num_indices, tctx.temp);


    // Reserve arrays of attributes up to maximum index count.
    Array<vec3> positions;
    Array<vec2> uvs;
    Array<vec3> normals;
    Array<vec4> tangents;

    positions.allocator = tctx.temp;
    uvs.allocator       = tctx.temp;
    normals.allocator   = tctx.temp;
    tangents.allocator  = tctx.temp;

    array_reserve(&positions, num_indices);
    array_reserve(&uvs,       num_indices);
    array_reserve(&normals,   num_indices);
    array_reserve(&tangents,  num_indices);


    // Reserves a temporary buffer with size of a maximum indices 
    // a triangulated face might hold in this mesh.
    s64 num_indices_in_face = mesh->max_face_triangles * 3;
    u32 *indices_in_face    = (u32*)alloc(sizeof(u32) * num_indices_in_face, tctx.temp);


    Array<Mesh_Part> parts;
    parts.allocator = tctx.temp;
    array_reserve(&parts, mesh->material_parts.count);


    for (ufbx_mesh_part part : mesh->material_parts)
    {
      Mesh_Part p = {};
      p.offset = positions.count;

      for (u32 face_index : part.face_indices)
      {
        ufbx_face face = mesh->faces[face_index];
        s64 num_tri_in_face = ufbx_triangulate_face(indices_in_face, num_indices_in_face, mesh, face);
        for (s64 i = 0; i < num_tri_in_face*3; ++i)
        {
          // Index
          u32 index = indices_in_face[i];

          // Position
          vec3 position = vec3_from_ufbx(ufbx_get_vertex_vec3(&mesh->vertex_position, index));
          append(&positions, position);

          // Unlike from normals and tangents, UV isn't 
          // generated even if it doesn't exist. 
          // @Todo: use xatlas or something and generate uv
          vec2 uv = has_uv ? vec2_from_ufbx(ufbx_get_vertex_vec2(&mesh->vertex_uv, index)) : vec2{};
          append(&uvs, uv);

          // Normal
          vec3 normal = {};
          if (has_normal)
          {
            normal = vec3_from_ufbx(ufbx_get_vertex_vec3(&mesh->vertex_normal, index));
            append(&normals, normal);
          }
        }
      }


      ufbx_material *material = node->materials[part.index];
      String *material_path = table_find_pointer(&material_pointer_to_path, material);
      if (material_path)
      {
        p.material_path = *material_path;
      }


      p.count = positions.count - p.offset;
      append(&parts, p);
    }
    R_ASSERT(positions.count == num_indices);


    // Generate normals and tangents
    if (!has_normal && options.generate_normals_with_mesh_optimizer)
    {
      float crease_angle = radian_from_degree(60.f);
      float smoothing    = 3.f;
      meshopt_generateNormals((float*)normals.data, 
                              NULL, num_indices,
                              (float*)positions.data, positions.count, sizeof(positions.data[0]),
                              crease_angle, smoothing);
    }

    // I'll just generate tangents myself, without ufbx, 
    // so I won't worry about handedness.
    if (has_uv)
    {
      meshopt_generateTangents((float*)tangents.data, 
                               NULL, num_indices, 
                               (float*)positions.data, positions.count, sizeof(positions.data[0]), 
                               (float*)normals.data, sizeof(normals.data[0]), 
                               (float*)uvs.data, sizeof(uvs.data[0]),
                               0u);
    }
    else
    {
      log_warning(S("Mesh %s doesn't have UVs, so tangent generation will be skipped."), mesh->name.data);
    }


    // Write attributes
    for (s64 i = 0; i < num_indices; ++i)
    {
      Vertex *vertex = &vertices[i]; 
      memcpy(vertex->position, &positions.data[i], 12);
      memcpy(vertex->uv,       &uvs.data[i],        8);
      memcpy(vertex->normal,   &normals.data[i],   12);
      memcpy(vertex->tangent,  &tangents.data[i],  16);
    }


    //
    // Afterwards is optimizations, and meshoptimizer does all the heavy work.
    //
    // 1. Indexing
    //
    //    Since we reserved vertices up to the number of indices,
    //    it's time to deduplicate vertices.
    //
    u32 *remap = (u32*)alloc(sizeof(u32)*num_indices, tctx.temp);
    s64 num_vertices = meshopt_generateVertexRemap(remap,
                                                   NULL, num_indices,
                                                   vertices, num_indices, sizeof(Vertex));

    u32 *new_indices = (u32*)alloc(sizeof(u32)*num_indices, allocator);
    meshopt_remapIndexBuffer(new_indices, NULL, num_indices, remap);

    Vertex *new_vertices = (Vertex*)alloc(sizeof(Vertex)*num_vertices, allocator);
    meshopt_remapVertexBuffer(new_vertices, vertices, num_indices, sizeof(Vertex), remap);


    //
    // Fill in the mesh
    //
    m.name = copy_string(utf8c((u8*)node->name.data), allocator);

    m.num_vertices = num_vertices;
    m.vertices     = new_vertices;

    m.num_indices  = num_indices;
    m.indices      = new_indices;

    m.num_parts    = parts.count;
    m.parts        = (Mesh_Part*)alloc(sizeof(Mesh_Part)*m.num_parts, allocator);

    for (u32 i = 0; i < parts.count; ++i)
    {
      m.parts[i] = parts[i];
    }

    append(out_meshes, m);
  }


  // Recursion
  ufbx_node_list children = node->children;
  for (int i = 0; i < children.count; ++i) 
  {
    fbx_import_mesh_internal(children.data[i], options, out_meshes, material_pointer_to_path, allocator);
  }
}

static Array<Triangle_Mesh>
fbx_import_mesh(ufbx_node*                                        node, 
                FBX_Import_Options                                options, 
                Table<ufbx_material*, String, _util_hash_pointer> material_pointer_to_path,
                Allocator                                         allocator)
{
  Array<Triangle_Mesh> meshes;
  meshes.allocator = allocator;

  fbx_import_mesh_internal(node, options, &meshes, material_pointer_to_path, allocator);

  return meshes;
}

//
// Returns an array of skeletons and a boolean 
// representing whether it was successful.
//
static Pair<Array<Skeleton>, b32>
fbx_import_skeleton(ufbx_scene *scene, Allocator allocator)
{
  Array<Skeleton> skeletons = {};
  skeletons.allocator = allocator;

  for (int i = 0; i < scene->bones.count; ++i)
  {
    ufbx_bone *bone = scene->bones[i];
    if (!bone->is_root)
      continue;

    // We found the root bone of a skeleton.
    // Now we can make a skeleton.
    Array<Joint> joints = {};
    joints.allocator = tctx.temp;

    if (bone->instances.count == 0)
    {
      log_error(S("No instance of the bone: '%s' was found."), bone->name.data);
      return {{}, false};
    }

    ufbx_node *root_bone_node = bone->instances[0];

    // BFS 
    s32 self_index    = -1;
    s32 unnamed_index = 0;

    Queue <Pair<ufbx_node*, s32>> q;
    q.array.allocator = tctx.temp;
    queue_push(&q, {root_bone_node, -1});

    while (q.count != 0)
    {
      auto [node, parent] = queue_front(&q);
      queue_pop(&q);

      Joint joint = {};
      joint.parent              = parent;
      joint.local_transform     = m4x4_from_ufbx_matrix(node->node_to_parent);
      joint.inverse_bind_matrix = m4x4_from_ufbx_matrix(node->geometry_to_node);

      // @Todo: name
      if (!node->name.data || node->name.length == 0)
      {
        String unnamed = tprint(S("unnamed_%d"), unnamed_index++);
      }
      else
      {
      }

      append(&joints, joint);

      // This is now my index in the bone list, which will be 
      // passed to my children.
      self_index += 1;

      for (s32 ch = 0; ch < node->children.count; ++ch)
      {
        queue_push(&q, {node->children[ch], self_index});
      }
    }

    // Add the skeleton to the array to be returned.
    append(&skeletons, {});
    Skeleton *skeleton = &skeletons.data[skeletons.count-1];
    skeleton->joints = (Joint*)alloc(sizeof(Joint) * joints.count, allocator);
    skeleton->num_joints = joints.count;
  }

  return {skeletons, true};
}

b32
fbx_import(String filepath, 
           String output_filepath,
           FBX_Import_Options options)
{
  b32 result = false;

  // Check if the input file exists.
  if (!file_exists(filepath))
  {
    log_error(S("Input file: '%S' doesn't exit."), filepath);
    return false;
  }

  if (!output_filepath)
  {
    String name;
    s64 index = find_index_of_any_from_right(filepath, S("/\\"));
    if (index == -1) {
      // It is itself a filename
      name = filepath;
    } else  {
      name = slice(filepath, index + 1, filepath.len - index - 1);
    }
    name = path_strip_extension(name);
    output_filepath = tprint(S("%S/%S.triangle_mesh"), shared->data_path, name); // @Temporary
  }

  int index = 0;
  while (file_exists(output_filepath))
  {
    if (options.overwrite_existing_file)
      break;

    output_filepath = path_strip_extension(output_filepath);
    output_filepath = tprint(S("%S_%d.txt"), output_filepath, index);
    index += 1;
  }

  ufbx_load_opts opts = {};
  opts.target_unit_meters          = 1.0f;
  opts.target_axes                 = ufbx_axes_right_handed_y_up;
  opts.target_camera_axes          = ufbx_axes_right_handed_y_up;
  opts.target_light_axes           = ufbx_axes_right_handed_y_up;
  opts.geometry_transform_handling = UFBX_GEOMETRY_TRANSFORM_HANDLING_MODIFY_GEOMETRY; // bakes it into node
  opts.inherit_mode_handling       = UFBX_INHERIT_MODE_HANDLING_IGNORE;
  opts.generate_missing_normals    = true;
  opts.evaluate_skinning           = true;
  opts.clean_skin_weights          = true;
  opts.normalize_normals           = true;

  if (options.generate_normals_with_mesh_optimizer) {
    opts.generate_missing_normals = false;
  }

  ufbx_error error;
  ufbx_scene *scene = ufbx_load_file((const char*)filepath.str, &opts, &error);

  if (!scene) 
  {
    log_error(S("Failed to load fbx scene from '%S'. Description from ufbx: %s"), filepath, error.description.data);
    return result;
  }


  // @Temporary
  Allocator allocator = tctx.temp;


  // Import skeletons
  auto [skeletons, skeletons_success] = fbx_import_skeleton(scene, allocator);
  if (skeletons_success)
  {
    log_info(S("Imported %d skeletons."), skeletons.count);
  }

  // Import materials
  auto material_pointer_to_path = fbx_import_materials(scene);

  // Import meshes
  Array<Triangle_Mesh> meshes = fbx_import_mesh(scene->root_node, options, material_pointer_to_path, allocator);
  String s = asset_string_from_meshes(meshes, tctx.temp);



  // @Temporary
  File file = file_open(output_filepath, true);
  if (file_is_valid(file))
  {
    if (file_write(file, s.str, s.len))
    {
      result = true;
      log_info(S("Written to file: '%S'"), output_filepath);
    }
    else
    {
      log_error(S("Failed to write to file: '%S'"), output_filepath);
    }
  }
  else
  {
    log_error(S("Failed to open file: '%S'"), output_filepath);
  }
  file_close(&file);

  ufbx_free_scene(scene);

  return result;
}
