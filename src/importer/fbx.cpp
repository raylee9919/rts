// Copyright Seong Woo Lee. All Rights Reserved.

#include "./fbx.h"
#include "os/os.h"
#include "basic/log.h"
#include "basic/queue.h"
#include "basic/string_builder.h"
#include "math/math.h"
#include "mesh.h"
#include "material/material.h"
#include "third_party/ufbx/ufbx.h"
#include "third_party/meshoptimizer/meshoptimizer.h"

/*
   @Todos: 

   - Cleanup filename creation. I've added an option whether I'll 
     overwrite the existing file or not.

   - There are common parts in writing procedures. Might be collapsible.
  
   - More optimizations
  
   - Meshlet asset? (Low priority)
   
   - Revise the long arse comment right below.
*/

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

                      mesh
                        \
                    skin_deformers
                    : binding between a set of bones and a mesh
                            \
                       skin_clusters
                       : vertices bound to a single bone
*/


struct Import_State 
{
  ufbx_scene *scene;
  String      directory; // Relative directory where output files will be written.
  String      basename;  // Base name used to name output files, derived from the input file.
};

static m4x4
_to_m4x4(ufbx_matrix& m)
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
_to_vec2(ufbx_vec2 v)
{
  return vec2((f32)v.x, (f32)v.y);
}

static vec3
_to_vec3(ufbx_vec3 v)
{
  return vec3((f32)v.x, (f32)v.y, (f32)v.z);
}

static u32
_hash_ptr(ufbx_material *ptr)
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
static Table<ufbx_material*, String, _hash_ptr>
fbx_import_materials(ufbx_scene *scene)
{
  Table<ufbx_material*, String, _hash_ptr> result = {};
  result.allocator = tctx.temp;
 
  for (ufbx_material *material : scene->materials)
  {
    String_Builder sb = {};
    init(&sb, tctx.temp);
    {
      append(&sb, S("1 // Version\n\n"));

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
          String path_to_write = texture_file;
          if (!file_exists(texture_file))
          {
            path_to_write = path_filename(texture_file);
            log_warning(S("Texture: %`%S` doesn't exit. Will write as `./%S`"), texture_file, path_to_write);
          }
          append(&sb, tprint(S("%S : @\"./%S\"\n"), type_strings[i].string, path_to_write));
        }
      }
    }

    String s_ = flush(&sb);

    String path = tprint(S("./%.*s.material"), (int)material->name.length, material->name.data);
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

// `.visible` field isn't enough.
// Should check parents as well.
static b32
is_visible(ufbx_node *node)
{
  while (node)
  {
    ufbx_node *parent = node->parent;
    if (!node->visible) return false;
    node = parent;
  }
  return true;
}

static void
fbx_import_mesh_internal(ufbx_node*                               node, 
                         FBX_Import_Options                       options, 
                         Array<Triangle_Mesh>*                    out_meshes,
                         Table<ufbx_material*, String, _hash_ptr> material_pointer_to_path,
                         Allocator                                allocator)
{
  ufbx_mesh *mesh = node->mesh;
  if (mesh && 
      node->attrib_type == UFBX_ELEMENT_MESH && 
      is_visible(node))
  {
    Triangle_Mesh m = {};

    // Check if attributes exist. If it doesn't, we'll create manually.
    b32 has_normal    = mesh->vertex_normal.exists;
    b32 has_uv        = mesh->vertex_uv.exists;


    // Find skin
    ufbx_skin_deformer *skin = mesh->skin_deformers.count ? mesh->skin_deformers[0] : NULL;


    // Number of triangles if the mesh is fully triangulated.
    s64 num_tri = mesh->num_triangles;
    s64 num_indices = num_tri * 3;


    // Reserve arrays of attributes up to maximum index count.
    Array<vec3> positions(tctx.temp, num_indices);
    Array<vec2>       uvs(tctx.temp, num_indices);
    Array<vec3>   normals(tctx.temp, num_indices);
    Array<vec4>  tangents(tctx.temp, num_indices);


    // Reserve only if skinned
    struct int32x4 {
      s32 e[4];
    };
    Array<int32x4> joints;
    Array<vec4> weights;
    if (skin) {
      joints  = Array<int32x4>(tctx.temp, num_indices);
      weights = Array<vec4>(tctx.temp, num_indices);
    }


    // Make attributes
    Array<Vertex_Attribute> attributes(allocator, 6);
    append(&attributes, { VERTEX_ATTRIBUTE_POSITION, VERTEX_ATTRIBUTE_FLOAT32, 3, 12,  0, });
    append(&attributes, { VERTEX_ATTRIBUTE_UV,       VERTEX_ATTRIBUTE_FLOAT32, 2,  8, 12, });
    append(&attributes, { VERTEX_ATTRIBUTE_NORMAL,   VERTEX_ATTRIBUTE_FLOAT32, 3, 12, 20, });
    append(&attributes, { VERTEX_ATTRIBUTE_TANGENT,  VERTEX_ATTRIBUTE_FLOAT32, 4, 16, 32, });
    if (skin)
    {
      append(&attributes, { VERTEX_ATTRIBUTE_JOINT,     VERTEX_ATTRIBUTE_INT32, 4, 16, 48, });
      append(&attributes, { VERTEX_ATTRIBUTE_WEIGHT,  VERTEX_ATTRIBUTE_FLOAT32, 4, 16, 64, });
    }


    // Reserve vertices upfront. Will be deduplicated afterwards.
    s64 vertex_size = vertex_size_from_attributes(&attributes);
    u8 *vertices = alloc(vertex_size*num_indices, tctx.temp);


    // Reserves a temporary buffer with size of a maximum indices 
    // a triangulated face might hold in this mesh.
    s64 num_indices_in_face = mesh->max_face_triangles * 3;
    u32 *indices_in_face    = (u32*)alloc(sizeof(u32) * num_indices_in_face, tctx.temp);

    Array<Mesh_Part> parts(tctx.temp, mesh->material_parts.count);

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
          ufbx_vec3 geomtry_space_position = ufbx_get_vertex_vec3(&mesh->vertex_position, index);
          vec3 world_space_position = _to_vec3(ufbx_transform_position(&node->geometry_to_world, geomtry_space_position));
          append(&positions, world_space_position);

          // Unlike from normals and tangents, UV isn't 
          // generated even if it doesn't exist. 
          // @Todo: use xatlas or something and generate uv
          vec2 uv = has_uv ? _to_vec2(ufbx_get_vertex_vec2(&mesh->vertex_uv, index)) : vec2{};
          append(&uvs, uv);

          // Normal
          if (has_normal)
          {
            ufbx_vec3 geometry_space_normal = ufbx_get_vertex_vec3(&mesh->vertex_normal, index);
            vec3 world_space_normal = _to_vec3(ufbx_transform_direction(&node->geometry_to_world, geometry_space_normal));
            append(&normals, world_space_normal);
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
      u8 *vertex = vertices + vertex_size*i;
      memcpy(vertex +  0, &positions.data[i], 12);
      memcpy(vertex + 12,       &uvs.data[i],  8);
      memcpy(vertex + 20,   &normals.data[i], 12);
      memcpy(vertex + 32,  &tangents.data[i], 16);
      if (skin)
      {
        memcpy(vertex + 48,  &joints.data[i], 16);
        memcpy(vertex + 64, &weights.data[i], 16);
      }
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
                                                   vertices, num_indices, vertex_size);

    u32 *new_indices = (u32*)alloc(sizeof(u32)*num_indices, allocator);
    meshopt_remapIndexBuffer(new_indices, NULL, num_indices, remap);

    u8 *new_vertices = alloc(vertex_size*num_vertices, allocator);
    meshopt_remapVertexBuffer(new_vertices, vertices, num_indices, vertex_size, remap);


    //
    // Fill in the mesh
    //
    m.name = copy_string(utf8c((u8*)node->name.data), allocator);

    m.attributes   = attributes;
    m.num_vertices = num_vertices;
    m.vertices     = new_vertices;
    m.vertex_size  = vertex_size;

    m.num_indices  = num_indices;
    m.indices      = new_indices;

    m.num_parts    = parts.count;
    m.parts        = (Mesh_Part*)alloc(sizeof(Mesh_Part)*m.num_parts, allocator);

    for (u32 i = 0; i < parts.count; ++i)
    {
      m.parts[i] = parts[i];
    }

    AABB aabb = {};
    if (positions.count > 0)
    {
      aabb.min = vec3( F32_MAX);
      aabb.max = vec3(-F32_MAX);
      for (u32 i = 0; i < positions.count; ++i)
      {
        vec3 p = positions.data[i];
        aabb.min.x = min(aabb.min.x, p.x);
        aabb.min.y = min(aabb.min.y, p.y);
        aabb.min.z = min(aabb.min.z, p.z);
        aabb.max.x = max(aabb.max.x, p.x);
        aabb.max.y = max(aabb.max.y, p.y);
        aabb.max.z = max(aabb.max.z, p.z);
      }
      vec3 padding = vec3(0.01f);
      aabb.min -= padding;
      aabb.max += padding;
    }
    m.aabb = aabb;

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
fbx_import_mesh(ufbx_node*                               node, 
                FBX_Import_Options                       options, 
                Table<ufbx_material*, String, _hash_ptr> material_pointer_to_path,
                Allocator                                allocator)
{
  Array<Triangle_Mesh> meshes;
  meshes.allocator = allocator;

  fbx_import_mesh_internal(node, options, &meshes, material_pointer_to_path, allocator);

  return meshes;
}

// Breadth-First Search the scene and return
// an array with root bone nodes. I'm doing this 
// because I don't trust `bone.is_root`.
static Array<ufbx_node *>
_get_root_bone_nodes(ufbx_scene *scene)
{
  Array<ufbx_node *> result = {};
  result.allocator = tctx.temp;

  Queue<ufbx_node*> q = {};
  q.array.allocator = tctx.temp;
  queue_push(&q, scene->root_node);

  while (q.count != 0)
  {
    ufbx_node *node = queue_pop(&q);
    if (node->bone)
    {
      append(&result, node);
      continue;
    }

    for (ufbx_node *child : node->children)
    {
      queue_push(&q, child);
    }
  }

  return result;
}

// Depth-First Search and iteratively collect 
// bones nodes and it's parent index. Won't 
// append non-bone nodes. 
static void
_collect_bones(ufbx_node *node, 
               Array<Pair<ufbx_node*, s32>> *result, 
               s32 parent)
{
  if (node->bone)
  {
    append(result, { node, parent });
    parent = (s32)result->count - 1;
  }

  for (ufbx_node *child : node->children)
  {
    _collect_bones(child, result, parent);
  }
}

static b32
_find_bind_to_world(ufbx_scene *scene, ufbx_node *node, ufbx_matrix *out)
{
  for (ufbx_skin_cluster *cluster : scene->skin_clusters)
  {
    if (cluster->bone_node == node)
    {
      *out = cluster->bind_to_world;
      return true;
    }
  }

  if (node->bind_pose)
  {
    ufbx_bone_pose *pose = ufbx_get_bone_pose(node->bind_pose, node);
    if (pose)
    {
      *out = pose->bone_to_world;
      return true;
    }
  }

  return false;
}

static Array<Skeleton>
fbx_import_skeleton(Import_State *state, Allocator allocator)
{
  Array<Skeleton> skeletons = {};
  skeletons.allocator = allocator;

  Array<ufbx_node *> root_bones = _get_root_bone_nodes(state->scene);

  for (ufbx_node *root : root_bones)
  {
    // Collect bones consisting a skeleton 
    // starting from the root bone.
    Array<Pair<ufbx_node*, s32>> bones = {};
    bones.allocator = tctx.temp;
    _collect_bones(root, &bones, -1);


    Skeleton skel = {};
    skel.joints = (Joint*)alloc(sizeof(Joint) * bones.count, allocator);
    skel.num_joints = bones.count;

    ufbx_matrix *bind_world     = (ufbx_matrix*)alloc(sizeof(ufbx_matrix) * bones.count, tctx.temp);
    ufbx_matrix *inv_bind_world = (ufbx_matrix*)alloc(sizeof(ufbx_matrix) * bones.count, tctx.temp);

    for (u32 i = 0; i < bones.count; ++i)
    {
      auto       pair = bones[i];
      ufbx_node *node = pair.x;
      s32      parent = pair.y;

      if (!_find_bind_to_world(state->scene, node, &bind_world[i]))
      {
        if (parent < 0)
        {
          R_ASSERT(parent == -1);
          bind_world[i] = node->node_to_world;
        }
        else
        {
          ufbx_matrix parent_world_inv = ufbx_matrix_invert(&bones[parent].x->node_to_world);
          ufbx_matrix rel              = ufbx_matrix_mul(&parent_world_inv, &node->node_to_world);
          bind_world[i]                = ufbx_matrix_mul(&bind_world[parent], &rel);
        }
      }

      inv_bind_world[i] = ufbx_matrix_invert(&bind_world[i]);

      ufbx_matrix local = (parent < 0) ? bind_world[i] : ufbx_matrix_mul(&inv_bind_world[parent], &bind_world[i]);

      // Fill in the joint
      Joint *joint = &skel.joints[i];
      joint->name                = copy_string(_to_string(node->name), allocator);
      joint->parent              = parent;
      joint->local_transform     = _to_m4x4(local);
      joint->inverse_bind_matrix = _to_m4x4(inv_bind_world[i]);
    }

    append(&skeletons, skel);
  }

  return skeletons;
}

/*
  Write out skeleton files.
  If there are multiple skeletons, filenames are prefixed 
  by numbers starting from `0`.
*/
static void
write_skeleton_files(Import_State *state, Array<Skeleton> skeletons)
{
  for (u32 i = 0; i < skeletons.count; ++i)
  {
    String filename;
    if (skeletons.count == 1)
    {
      filename = tprint(S("%S.skeleton"), state->basename);
    }
    else
    {
      filename = tprint(S("%S_%u.skeleton"), state->basename, i);
    }

    String strings[] = {
      state->directory, filename
    };
    String path = path_join(strings, 2, '/', tctx.temp);


    File file = file_open(path, true);
    if (file_is_valid(file))
    {
      String s = asset_string_from_skeleton(&skeletons.data[i], tctx.temp);
      if (file_write(file, s.str, s.len))
      {
        log_info(S("Written skeleton to file: '%S'"), path);
      }
      else
      {
        log_error(S("Failed to write skeleton to file: '%S'"), path);
      }
    }
    else
    {
      log_error(S("Failed to open file: '%S' during importing skeleton."), path);
    }
    file_close(&file);
  }
}

static void
write_mesh_files(Import_State *state, Array<Triangle_Mesh> meshes)
{
  String filename;
  filename = tprint(S("%S.triangle_mesh"), state->basename);

  String strings[] = {
    state->directory, filename
  };
  String path = path_join(strings, 2, '/', tctx.temp);


  File file = file_open(path, true);
  if (file_is_valid(file))
  {
    String s = asset_string_from_meshes(meshes, tctx.temp);
    if (file_write(file, s.str, s.len))
    {
      log_info(S("Written meshes to file: '%S'"), path);
    }
    else
    {
      log_error(S("Failed to write meshes to file: '%S'"), path);
    }
  }
  else
  {
    log_error(S("Failed to open file: '%S' during importing meshes."), path);
  }
  file_close(&file);
}

b32
fbx_import(String             filepath, 
           String             out_directory,
           FBX_Import_Options options)
{
  if (!file_exists(filepath))
  {
    log_error(S("Input file: '%S' doesn't exit."), filepath);
    return false;
  }

  ufbx_load_opts opts = {};
  opts.target_unit_meters          = 1.0f;
  opts.target_axes                 = ufbx_axes_right_handed_y_up;
  opts.target_camera_axes          = ufbx_axes_right_handed_y_up;
  opts.target_light_axes           = ufbx_axes_right_handed_y_up;
  opts.geometry_transform_handling = UFBX_GEOMETRY_TRANSFORM_HANDLING_MODIFY_GEOMETRY; // bakes it into node
  opts.inherit_mode_handling       = UFBX_INHERIT_MODE_HANDLING_IGNORE;
  opts.generate_missing_normals    = true;
  opts.clean_skin_weights          = true;
  opts.normalize_normals           = true;

  if (options.generate_normals_with_mesh_optimizer) {
    opts.generate_missing_normals = false;
  }


  // Load scene
  ufbx_error error;
  ufbx_scene *scene = ufbx_load_file((const char*)filepath.str, &opts, &error);
  if (!scene) 
  {
    log_error(S("Failed to load fbx scene from '%S'. Description from ufbx: %s"), filepath, error.description.data);
    return false;
  }


  // Make import state
  Import_State state = {};
  state.scene     = scene;
  state.directory = out_directory;
  state.basename  = path_basename(filepath);


  // 
  Allocator allocator = tctx.temp;


  // Import skeletons
  Array<Skeleton> skeletons = fbx_import_skeleton(&state, allocator);
  write_skeleton_files(&state, skeletons); // @Todo: Prune unreferenced skeletons first


  // Import materials
  auto material_pointer_to_path = fbx_import_materials(scene);


  // Import meshes
  Array<Triangle_Mesh> meshes = fbx_import_mesh(scene->root_node, options, material_pointer_to_path, allocator);
  write_mesh_files(&state, meshes);

  ufbx_free_scene(scene);

  return true;
}
