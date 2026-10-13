// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RTS_MESH_H
#define RTS_MESH_H

#include "basic/array.h"
#include "basic/string.h"
#include "os/os.h"
#include "math/math.h"
#include "asset/asset.h"

struct Skeleton;
struct Asset_Lexer;

struct Vertex
{
  float position[3];
  float uv[2];
  float normal[3];
  float tangent[4];
};

struct Mesh_Part
{
  // offset and count into index buffer
  s64 offset;
  s64 count;
  
  String material_path;
};

struct Triangle_Mesh 
{
  String     name;

  s64        num_vertices;
  Vertex    *vertices;

  s64        num_indices;
  u32       *indices;

  s64        num_parts;
  Mesh_Part *parts;

  AABB       aabb;
};

String
asset_string_from_skeleton(Skeleton *sk, 
                           Allocator allocator);

String
asset_string_from_meshes(Array<Triangle_Mesh> meshes, 
                         Allocator            allocator);

Array<Triangle_Mesh>
parse_and_eval_mesh_asset(Asset_Lexer *lexer, 
                          Allocator    allocator);

ASSET_LOAD_PROC(mesh_load_proc);

//
// S k e l e t o n
//
struct Joint 
{
  String name;
  s32    parent; // Always precedes the child. -1 for the root
  m4x4   local_transform;
  m4x4   inverse_bind_matrix;
};

struct Skeleton 
{
  Joint *joints;
  s64    num_joints;
};

ASSET_LOAD_PROC(skeleton_load_proc);



#endif // RTS_MESH_H
