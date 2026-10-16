// Copyright Seong Woo Lee. All Rights Reserved.

#include "./fbx.h"
#include "os/os.h"
#include "basic/log.h"
#include "basic/queue.h"
#include "basic/string_builder.h"
#include "math/math.h"
#include "mesh.h"
#include "animation.h"
#include "material/material.h"
#include "third_party/ufbx/ufbx.h"
#include "third_party/meshoptimizer/meshoptimizer.h"

/*
   @Todos: 

   - Cleanup filename creation. I've added an option whether I'll 
     overwrite the existing file or not.
   
   - Proper null-skin weights and indices.

   - Sanitize names

   - Duplicate bone names

   - There are common parts in file-writing procedures. Might be collapsible.
  
   - Semantic name ("position") is a promise between assets and the engine. 
     So, should I need to bake offset/size information in the asset?

   - More optimizations

   - Find way for LoD generation

   - Meshlet asset for mesh shader? Sounds fun, but low priority.
*/

/*
  There are basically 2 options for how to import skeletal meshes:

  1. geometry space vertices + per-mesh matrix that converts vertices to 'root bone node'  space
  2. geometry space vertices +                    ..                     'scene root node' space

  The core part of mesh skinning is, vertices and bones must be in same space at some point for mesh skinning to work.


  1.

         composite_matrix[n] = global_matrix * global_transform[n] * node_from_root * mesh.root_from_geometry * vertex_in_geometry_space
                                     |                                     |                    |
                                     |                                     |                    |
                         additional multiplication (1)                     |        additional multiplication (2)
                                                                   inverse rest pose 
                                                                   against root bone
     

  2.

         composite_matrix[n] = global_matrix * global_transform[n] * node_from_global * mesh.global_from_geometry * vertex_in_geometry_space
                                                                            |
                                                                    inverse rest pose 
                                                                 against scene root node
     
     Two additional matrix multiplcations stay the same.



  
  First, we'll deal with (2):

      In order to remove (2), vertices should be baked from 'geometry' to (   ) space, where (   ) could be either 'global' or 'root bone'.

      (a) bake as `global` space vertices:

          geometry_to_global = cluster.bind_to_world * cluster.geometry_to_bone
          node_from_global   = inverse(bone_node.node_to_world)

      (b) bake as `root bone` space vertices:

          geometry_to_root_bone = inverse(root_bone.node_to_world) * cluster.bind_to_world * cluster.geometry_to_bone
          node_from_root_bone   = inverse(node.node_to_world) * root_bone_node.node_to_world

      Both are fine. The difference in complexity is negligible.
      Good to know that vertex baking is flexible! We have some room to adjust this part once we've looked at the other parts.


      or


      (c) leave vertices in geometry space, and just pass `mesh.(   )_from_geometry` to shader.
          One additional matrix multiplication on GPU is nothing.

          So, this part:

              mesh.(   )_from_geometry * vertex_in_geometry_space

          is done on GPU.

      I'd prefer (a) or (b). I don't see a good reason not to bake it into vertices.

  
  Now, (1):

      composite_matrix[n] = global_matrix * global_transform[n]
                          = (global from root bone) * (root bone from root + 1) * ... * (parent_of(n) from node n)

      What if we collapse   |-------------------------------------------------| part?
                                                  T H I S

      That is, make root bone node incorporate all the transforms coming from its ancestors.

      Neat, but here's the problem:

          `ufbx_evaluate_transform(t)` gives nodes local transform, relative to parent, at time t.
          And for every evaluated samples in the animation, the following must be done:

              (global from (root - 1)) * (evaluated root bone transform)
                          
                          
      I don't know whether this part should be evaluated or not. If it doesn't have to be, 
      all I have to do is multiply by 'root_bone.parent ? root_bone.parent.node_to_world : identity()'

      If that's not the case, I should traverse from the root bone node to the scene root node an concatenate the evaluated transforms.
      As I'm not sure about my assumption, I'll just take the latter path.
*/



static u32
_hash_node(ufbx_node *ptr)
{
  u64 u = (uintptr_t)ptr;
  return knuth_hash(u);
}

struct Joint_Ref
{
  s32 skeleton_index_in_array;
  s32 joint_index;
};

struct Import_State 
{
  ufbx_scene *scene;
  String      directory; // Relative directory where output files will be written.
  String      basename;  // Base name used to name output files, derived from the input file.

  FBX_Import_Options options;

  //
  // Helper data structures
  //
  b8 *is_joint;
  Table<ufbx_node *, Joint_Ref, _hash_node> node_to_joint;
  Array<Skeleton> skeletons;
  Array<Array<ufbx_node*>> nodes_in_skeleton;
};

static m4x4
to_m4x4(ufbx_matrix& m)
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
to_vec2(ufbx_vec2 v)
{
  return vec2((f32)v.x, (f32)v.y);
}

static vec3
to_vec3(ufbx_vec3 v)
{
  return vec3((f32)v.x, (f32)v.y, (f32)v.z);
}

static Quaternion
to_quaternion(ufbx_quat q)
{
  return Quaternion(q.w, q.x, q.y, q.z);
}

static u32
_hash_ptr(ufbx_material *ptr)
{
  u64 u = (uintptr_t)ptr;
  return knuth_hash(u);
}

static String
to_string(ufbx_string c, Allocator allocator)
{
  String s = { (u8*)c.data, (s64)c.length };
  return copy_string(s, allocator);
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
          String texture_file = to_string(texture->filename, tctx.temp);
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
import_mesh_internal(ufbx_node*                               node, 
                     Import_State                            *state,
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
    ufbx_skin_deformer *skin = NULL;
    if (mesh->skin_deformers.count &&
        mesh->skin_deformers[0]->clusters.count && 
        mesh->skin_deformers[0]->weights.count)
    {
      skin = mesh->skin_deformers[0];
    }

    if (skin) {
      R_ASSERT(mesh->skin_deformers.count == 1); // @Robustness: PLEASE!!!
    }


    // Number of triangles if the mesh is fully triangulated.
    s64 num_tri = mesh->num_triangles;
    s64 num_indices = num_tri * 3;


    // Reserve arrays of attributes up to maximum index count.
    Array<vec3> positions(tctx.temp, num_indices);
    Array<vec2>       uvs(tctx.temp, num_indices);
    Array<vec3>   normals(tctx.temp, num_indices);
    Array<vec4>  tangents(tctx.temp, num_indices);


    // Reserve only if skinned
    struct s32x4 {
      s32 e[4];
    };
    Array<s32x4> joints;
    Array<vec4> weights;
    if (skin) {
      joints  = Array<s32x4>(tctx.temp, num_indices);
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


    // Matrix to transform vertices in 'geometry(geomtric) space' to 'world(global) space'.
    ufbx_matrix geometry_to_world;
    if (skin)
    {
      ufbx_skin_cluster *c = skin->clusters[0];
      geometry_to_world = ufbx_matrix_mul(&c->bind_to_world, &c->geometry_to_bone);
    }
    else
    {
      geometry_to_world = node->geometry_to_world;
    }

    // Matrix to transform normals
    ufbx_matrix normal_matrix = ufbx_matrix_for_normals(&geometry_to_world);


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
          vec3 world_space_position = to_vec3(ufbx_transform_position(&geometry_to_world, geomtry_space_position));
          append(&positions, world_space_position);

          // Unlike from normals and tangents, UV isn't 
          // generated even if it doesn't exist. 
          // @Todo: use xatlas or something and generate uv
          vec2 uv = has_uv ? to_vec2(ufbx_get_vertex_vec2(&mesh->vertex_uv, index)) : vec2{};
          append(&uvs, uv);

          // Normal
          if (has_normal)
          {
            ufbx_vec3 geometry_space_normal = ufbx_get_vertex_vec3(&mesh->vertex_normal, index);
            vec3 world_space_normal = to_vec3(ufbx_vec3_normalize(ufbx_transform_direction(&normal_matrix, geometry_space_normal)));
            append(&normals, world_space_normal);
          }

          // Skin
          if (skin)
          {
            u32 vertex = mesh->vertex_indices[index];
            ufbx_skin_vertex skin_vertex = skin->vertices[vertex];

            s32 _joints[4]   = {-1,-1,-1,-1};
            f32 _weights[4]  = {0,0,0,0};
            f32 total_weight = 0.f;

            s64 n = 0;
            for (s64 vi = 0; vi < skin_vertex.num_weights; ++vi)
            {
              if (n >= 4) break;

              ufbx_skin_weight skin_weight = skin->weights[skin_vertex.weight_begin + vi];
              ufbx_skin_cluster *cluster = skin->clusters[skin_weight.cluster_index];

              Joint_Ref *joint_ref = table_find_pointer(&state->node_to_joint, cluster->bone_node);
              if (!joint_ref) continue;

              Guid skel_id = state->skeletons[joint_ref->skeleton_index_in_array].id;
              R_ASSERT(m.skeleton == NULL_GUID || m.skeleton == skel_id);
              m.skeleton = skel_id;

              _joints[n]    = joint_ref->joint_index;
              _weights[n]   = skin_weight.weight;
              total_weight += skin_weight.weight;
              n += 1;
            }

            if (total_weight > 1e-5f)
            {
              f32 rcp = 1.f / total_weight;
              for (s64 i = 0; i < 4; ++i)
              {
                _weights[i] *= rcp;
              }
            }

            append(&joints, s32x4{_joints[0], _joints[1], _joints[2], _joints[3]});
            append(&weights, vec4(_weights[0], _weights[1], _weights[2], _weights[3]));
          }
        }
      }

      if (part.index < node->materials.count)
      {
        ufbx_material *material = node->materials[part.index];
        String *material_path = table_find_pointer(&material_pointer_to_path, material);
        if (material_path)
        {
          p.material_path = *material_path;
        }
      }

      p.count = positions.count - p.offset;
      append(&parts, p);
    }
    R_ASSERT(positions.count == num_indices);


    // Generate normals and tangents
    if (!has_normal && state->options.generate_normals_with_mesh_optimizer)
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

    // @Robustness: AABB for skeletal meshes...
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
    import_mesh_internal(children.data[i], state, out_meshes, material_pointer_to_path, allocator);
  }
}

static Array<Triangle_Mesh> import_meshes(ufbx_node*                               node, 
                                          Import_State                            *state,
                                          Table<ufbx_material*, String, _hash_ptr> material_pointer_to_path,
                                          Allocator                                allocator)
{
  Array<Triangle_Mesh> meshes;
  meshes.allocator = allocator;

  import_mesh_internal(node, state, &meshes, material_pointer_to_path, allocator);

  log_info(S("Imported '%d' %S."), meshes.count, meshes.count==1 ? S("mesh") : S("meshes"));

  return meshes;
}

static void
lift_multirooted_skins(ufbx_scene *scene, b8 *is_joint)
{
  for (ufbx_skin_deformer *skin : scene->skin_deformers)
  {
    if (skin->clusters.count == 0 || skin->weights.count == 0)
      continue;

    //
    // 1. Collect top-most nodes in the deformer.
    //
    Array<ufbx_node*> roots(tctx.temp);
    for (ufbx_skin_cluster *c : skin->clusters)
    {
      if (!c->bone_node || !is_joint[c->bone_node->typed_id])
        continue;

      ufbx_node *r = c->bone_node;
      while (r->parent && is_joint[r->parent->typed_id])
        r = r->parent;

      if (array_find(&roots, r, [](ufbx_node *a, ufbx_node *b) { return a==b; }) < 0) {
        append(&roots, r);
      }
    }

    if (roots.count <= 1) continue; // no other chains to lift up.

    //
    // 2. Mark nodes from each root up to the shallowest root's depth
    //    and update roots accordingly.
    //
    u32 min_depth = INT32_MAX;
    for (u32 i = 0; i < roots.count; ++i) {
      min_depth = min(min_depth, roots[i]->node_depth);
    }

    for (u32 i = 0; i < roots.count; ++i) {
      while (roots[i]->node_depth > min_depth) {
        roots[i] = roots[i]->parent;
        is_joint[roots[i]->typed_id] = 1;
      }
    }

    //
    // 3. Lift roots up until they share the same parent.
    //
    while (1)
    {
      b32 same = true;
      for (u32 i = 1; i < roots.count; ++i) 
      {
        if (roots[i]->parent != roots[0]->parent)
        {
          same = false;
          break;
        }
      }
      if (same) break;

      for (u32 i = 0; i < roots.count; ++i) 
      {
        R_ASSERT(roots[i]->parent);
        roots[i] = roots[i]->parent;
        is_joint[roots[i]->typed_id] = 1;
      }
    }
  }
}

static b8 *
_mark_joints(ufbx_scene *scene)
{
  s64 sz = sizeof(b8) * scene->nodes.count;
  b8 *is_joint = (b8*)alloc(sz, tctx.temp);
  memset(is_joint, 0, sz);

  // Mark 'node.bone'
  for (ufbx_node *node : scene->nodes) {
    if (node->bone) {
      is_joint[node->typed_id] = 1;
    }
  }

  // Mark nodes referenced by skin clusters
  for (ufbx_skin_deformer *skin : scene->skin_deformers) 
  {
    if (skin->clusters.count == 0 || skin->weights.count == 0) {
      continue;
    }

    for (ufbx_skin_cluster *c : skin->clusters) {
      if (c->bone_node) {
        is_joint[c->bone_node->typed_id] = 1;
      }
    }
  }


  /*
    [V]: marked as 'is_joint'

                    ...                ...        
                  /     \            /     \              
                 [V]   [ ]    -->   [V]   [V]                 
                 / \   / \          / \   / \                  
                     [V] [ ]            [V] [ ]                 
   
     There can be more than one trees in a skin deformer.
     In that case, we lift up the lower one until they share 
     the same parent.
   */
  lift_multirooted_skins(scene, is_joint);


  /*          
              [V]
              / \
            ..  [ ] <--- mark this as a joint as well,
                / \      or local transforms might be bogus.
              ..  [V]
   */
  Array<ufbx_node*> marked(tctx.temp);
  for (ufbx_node *node : scene->nodes) {
    if (is_joint[node->typed_id]) {
      append(&marked, node);
    }
  }

  for (ufbx_node *node : marked) {
    ufbx_node *ancestor = node->parent; 
    while (ancestor && !is_joint[ancestor->typed_id]) {
      ancestor = ancestor->parent;
    }

    if (!ancestor) continue;

    for (ufbx_node *p = node->parent; p != ancestor; p = p->parent) {
      is_joint[p->typed_id] = 1;
    }
  }


  /*
    Drop trees that deform nothing.
          
                        [V]        [V]           
                        / \        / \          
            IK_Heel  [V]        [ ]              
                     / \        / \               
            IK_Toe [V]        [ ]               
  */
  if (scene->skin_clusters.count > 0)
  {
    s64 n = scene->nodes.count;

    // node to its root bone node
    ufbx_node **root_of = (ufbx_node**)alloc(sizeof(ufbx_node*)*n, tctx.temp);
    for (ufbx_node *node : scene->nodes)
    {
      ufbx_node *r = NULL;
      if (is_joint[node->typed_id])
      {
        r = node;
        while (r->parent && is_joint[r->parent->typed_id])
          r = r->parent;
      }
      root_of[node->typed_id] = r;
    }


    b8 *deforms = (b8*)alloc(n, tctx.temp);
    memset(deforms, 0, n);

    for (ufbx_skin_deformer *skin : scene->skin_deformers)
    {
      if (skin->clusters.count == 0 || skin->weights.count == 0)
        continue;

      for (ufbx_skin_cluster *c : skin->clusters)
      {
        if (c->bone_node && is_joint[c->bone_node->typed_id])
          deforms[root_of[c->bone_node->typed_id]->typed_id] = 1;
      }
    }

    for (ufbx_node *node : scene->nodes)
    {
      ufbx_node *r = root_of[node->typed_id];
      if (r && !deforms[r->typed_id])
        is_joint[node->typed_id] = 0;
    }
  }

  return is_joint;
}

// Breadth-First Search the scene and return an array with root bone nodes, 
// becuase `bone.is_root` isn't enough.
//
static Array<ufbx_node *>
get_root_bones(const ufbx_scene *scene, 
               const b8         *is_joint)
{
  Array<ufbx_node *> result = {};
  result.allocator = tctx.temp;

  Queue<ufbx_node*> q = {};
  q.array.allocator = tctx.temp;
  queue_push(&q, scene->root_node);

  while (q.count != 0)
  {
    ufbx_node *node = queue_pop(&q);
    if (is_joint[node->typed_id])
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
_collect_bones(ufbx_node                    *node, 
               Import_State                 *state,
               Array<Pair<ufbx_node*, s32>> *result, 
               s32                           parent)
{
  if (state->is_joint[node->typed_id])
  {
    // Otherwise sampled local transforms don't compose with the joint hierarchy!
    R_ASSERT(parent < 0 || node->parent == (*result)[parent].x);

    append(result, { node, parent });
    parent = (s32)result->count - 1;
  }

  for (ufbx_node *child : node->children)
  {
    _collect_bones(child, state, result, parent);
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

static s32
_find(s32 *head, s32 x)
{
  while (head[x] != x) {
    x = head[x] = head[head[x]];
  }
  return x;
}

static void
_union(s32 *head, s32 a, s32 b)
{
  a = _find(head, a);
  b = _find(head, b);
  if (a != b) {
    head[b] = a;
  }
}

static s32
node_to_index_of_root_bone(ufbx_node *node, Array<ufbx_node*> root_bones)
{
  while (node) {
    // Am I a root bone?
    s32 idx = (s32)array_find(&root_bones, node, [](ufbx_node *a, ufbx_node *b) {
      return a == b;
    });

    if (idx >= 0) return idx; // yes

    node = node->parent; // no, go up
  }
  return -1;
}

static Array<Skeleton>
import_skeletons(Import_State *state, Allocator allocator)
{
  Array<Skeleton> skeletons = {};
  skeletons.allocator = allocator;

  // index by 'node.typed_id'
  state->is_joint = _mark_joints(state->scene);

  Array<ufbx_node *> root_bones = get_root_bones(state->scene, state->is_joint);

  //
  // Determine which subtrees belong to the same skeleton.
  // N subtree roots do not necessarily imply N skeletons,
  // which is exactly what we want. We use some wacky heuristics 
  // to determine how to group them.
  //

  /*
     1.
     Union roots with the same deformer.

         S: same skin deformer
         .  ..  .
         / \    / \
         S   .   S  . 
         / \ / \ / \ 

     Those will be written to a multi-rooted, single skeleton.
  */
  s32 *heads = (s32*)alloc(sizeof(s32)*root_bones.count, tctx.temp);
  for (u32 i = 0; i < root_bones.count; ++i) {
    heads[i] = i;
  }

  for (ufbx_skin_deformer *skin : state->scene->skin_deformers)
  {
    if (skin->clusters.count == 0 || skin->weights.count == 0)
      continue;

    s32 first = -1;
    for (ufbx_skin_cluster *c : skin->clusters)
    {
      if (!c->bone_node || !state->is_joint[c->bone_node->typed_id]) 
        continue;

      s32 root = node_to_index_of_root_bone(c->bone_node, root_bones);
      if (root < 0) continue;

      if (first < 0)
        first = root;
      else
        _union(heads, first, root);
    }
  }

  /* 
     2.
     Union roots that share the same parent,
     unless the parent is the scene root node.
  */
  for (u32 i = 0; i < root_bones.count; ++i) {
    for (u32 j = i + 1; j < root_bones.count; ++j) {
      ufbx_node *parent1 = root_bones[i]->parent;
      ufbx_node *parent2 = root_bones[j]->parent;
      if (parent1 && 
          parent1 == parent2 && 
          parent1 != state->scene->root_node) {
        _union(heads, i, j);
      }
    }
  }


  for (u32 h = 0; h < root_bones.count; ++h)
  {
    s32 head = _find(heads, h);
    b32 visited = false;
    for (u32 k = 0; k < h; ++k) {
      if (_find(heads, k) == head) 
        visited = true;
    }
    if (visited) continue;


    // Collect all the bones forming the skeleton 
    // by traversing from the root bone(s).
    Array<Pair<ufbx_node*, s32>> bones(tctx.temp);
    for (u32 i = 0; i < root_bones.count; ++i) {
      if (_find(heads, i) == head) {
        _collect_bones(root_bones[i], state, &bones, -1);
      }
    }

    // Collect nodes referenced by the current skeleton.
    // Used later while importing animations.
    Array<ufbx_node *> nodes(tctx.temp);
    for (u32 b = 0; b < bones.count; ++b) {
      ufbx_node *node = bones.data[b].x;
      append(&nodes, node);
    }


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
          ufbx_matrix parent_from_node = ufbx_matrix_mul(&parent_world_inv, &node->node_to_world);
          bind_world[i]                = ufbx_matrix_mul(&bind_world[parent], &parent_from_node);
        }
      }

      inv_bind_world[i] = ufbx_matrix_invert(&bind_world[i]);

      ufbx_matrix local = (parent < 0) ? bind_world[i] : ufbx_matrix_mul(&inv_bind_world[parent], &bind_world[i]);

      // Fill in the joint
      Joint *joint = &skel.joints[i];
      joint->name                = to_string(node->name, allocator);
      joint->parent              = parent;
      joint->local_transform     = to_m4x4(local);
      joint->inverse_bind_matrix = to_m4x4(inv_bind_world[i]);


      table_add(&state->node_to_joint, node, Joint_Ref{ (s32)skeletons.count, (s32)i });
    }

    append(&state->nodes_in_skeleton, nodes);
    append(&skeletons, skel);
  }

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
    Guid id = guid_from_string(path);
    skeletons[i].id = id;

    // @Todo: File watcher will replace this shit.
    table_add(&asset_system->guid_to_path, id, path);
  }

  log_info(S("Imported '%d' %S."), skeletons.count, skeletons.count==1 ? S("skeleton") : S("skeletons"));

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
    String path = asset_name_from_guid(skeletons[i].id);
    
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
write_animation_files(Import_State *state, Array<Sampled_Animation> animations)
{
  for (u32 i = 0; i < animations.count; ++i)
  {
    Sampled_Animation *anim = animations.data + i;
    String path = S("foo.sampled_animation");
    
    File file = file_open(path, true);
    if (file_is_valid(file))
    {
      String s = asset_string_from_animation(anim, tctx.temp);
      if (file_write(file, s.str, s.len))
      {
        log_info(S("Written animation to file: '%S'"), path);
      }
      else
      {
        log_error(S("Failed to write animation to file: '%S'"), path);
      }
    }
    else
    {
      log_error(S("Failed to open file: '%S' while trying to write animation file"), path);
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

static s32
_find_animated_skeleton(Import_State *state, ufbx_anim_stack *stack)
{
  s32 n = (s32)state->skeletons.count;
  s32 *hits = (s32*)alloc(sizeof(s32)*n, tctx.temp);
  memset(hits, 0, sizeof(s32)*n);

  for (ufbx_anim_layer *layer : stack->layers)
  {
    for (ufbx_anim_prop& prop : layer->anim_props)
    {
      ufbx_node *node = ufbx_as_node(prop.element);
      if (!node) continue;

      Joint_Ref *ref = table_find_pointer(&state->node_to_joint, node);
      if (ref) hits[ref->skeleton_index_in_array] += 1;
    }
  }

  s32 count = 0;
  for (s32 i = 0; i < n; ++i) {
    if (hits[i] > 0) count++;
  }
  if (count > 1) {
    R_ASSERT(!"More than one skeleton was touched by the animation stack."); // @Todo: Handle this... but how?
  }

  for (s32 i = 0; i < n; ++i) {
    if (hits[i] > 0) return i;
  }
  return -1;
}

static ufbx_matrix
_evaluate_global_matrix(ufbx_node *node, ufbx_anim *anim, f64 time)
{
  if (node == NULL) return ufbx_identity_matrix;

  ufbx_transform     transform = ufbx_evaluate_transform(anim, node, time);
  ufbx_matrix parent_from_self = ufbx_transform_to_matrix(&transform);

  ufbx_matrix world_from_parent = _evaluate_global_matrix(node->parent, anim, time);
  return ufbx_matrix_mul(&world_from_parent, &parent_from_self);
}

static Array<Sampled_Animation>
import_animations(Import_State *state, Allocator allocator)
{
  Array<Sampled_Animation> result(allocator);

  for (ufbx_anim_stack *stack : state->scene->anim_stacks)
  {
    f64 duration = stack->time_end - stack->time_begin;
    if (duration < 1e-6f) {
      log_warning(S("Animation: `%s` has near-zero duration: `%.12fs`, so skipped import."), stack->name.data, duration);
      continue;
    }

    // Find the skeleton to use for this animation.
    s64 skel_i = _find_animated_skeleton(state, stack);
    if (skel_i == -1) {
      log_warning(S("Animation: `%s` didn't touch any skeleton, so skipped import."), stack->name.data);
      continue;
    }
    Skeleton *skel = state->skeletons.data + skel_i;


    // All nodes referenced by the selected skeleton.
    // Ordered to match the skeleton's joint order.
    Array<ufbx_node*> nodes = state->nodes_in_skeleton[skel_i];


    Array<Animation_Sample> samples(allocator);

    f64 frame_rate = state->options.animation_frame_rate;
    s32 num_frames = (s32)ceil(duration * frame_rate - 1e-6) + 1;
    for (s32 f = 0; f < num_frames; ++f)
    {
      f64 t = min(stack->time_begin + (f64)f / frame_rate, stack->time_end);

      Animation_Sample sample = {};
      sample.xforms.allocator = allocator;

      for (u32 i = 0; i < nodes.count; ++i)
      {
        ufbx_node *node = nodes[i];
        ufbx_transform transform = ufbx_evaluate_transform(stack->anim, node, t);

        // Concatenate transforms coming from the scene root.
        if (skel->joints[i].parent < 0)
        {
          ufbx_matrix local_matrix  = ufbx_transform_to_matrix(&transform);
          ufbx_matrix concat_matrix = _evaluate_global_matrix(node->parent, stack->anim, t);
          ufbx_matrix final_matrix  = ufbx_matrix_mul(&concat_matrix, &local_matrix);
          transform = ufbx_matrix_to_transform(&final_matrix);
        }

        Xform xform(to_vec3(transform.translation), to_quaternion(transform.rotation), to_vec3(transform.scale));
        append(&sample.xforms, xform);
      }

      append(&samples, sample);
    }

    // Append
    Sampled_Animation anim = {};
    anim.skeleton   = skel->id;
    anim.num_joints = skel->num_joints;
    anim.frame_rate = (s32)frame_rate;
    anim.duration   = duration;
    anim.samples    = samples;
    append(&result, anim);
  }

  log_info(S("Imported '%d' %S."), result.count, result.count==1 ? S("animation") : S("animations"));

  return result;
}

static void
_print(ufbx_node *node, int depth)
{
  printf("                                  ");
  for (int i = 0; i < depth; ++i) printf("| ");
  printf("%s\n", node->name.data);

  for (ufbx_node *child : node->children) {
    _print(child, depth+1);
  }
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
  opts.clean_skin_weights          = true;
  opts.normalize_normals           = true;
  if (!options.generate_normals_with_mesh_optimizer) {
    opts.generate_missing_normals = true;
  }


  // Load scene
  ufbx_error error;
  ufbx_scene *scene = ufbx_load_file((const char*)filepath.str, &opts, &error);
  if (!scene) 
  {
    log_error(S("Failed to load fbx scene from '%S'. Description from ufbx: %s"), filepath, error.description.data);
    return false;
  }
  _print(scene->root_node, 0);


  // Make import state
  Import_State state = {};
  state.scene     = scene;
  state.directory = out_directory;
  state.basename  = path_basename(filepath);
  state.options   = options;
  state.node_to_joint.allocator = tctx.temp;
  state.nodes_in_skeleton.allocator = tctx.temp;


  // @Temporary
  Allocator allocator = tctx.temp;


  // Import skeletons
  Array<Skeleton> skeletons = import_skeletons(&state, allocator);
  state.skeletons = skeletons;
  write_skeleton_files(&state, skeletons);


  // Import animations
  Array<Sampled_Animation> animations = import_animations(&state, allocator);
  write_animation_files(&state, animations);

  // Import materials
  auto material_pointer_to_path = fbx_import_materials(scene);


  // Import meshes
  Array<Triangle_Mesh> meshes = import_meshes(scene->root_node, &state, material_pointer_to_path, allocator);
  write_mesh_files(&state, meshes);

  ufbx_free_scene(scene);

  return true;
}
