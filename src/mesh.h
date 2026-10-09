// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RTS_MESH_H
#define RTS_MESH_H

#include "basic/array.h"
#include "basic/string.h"
#include "os/os.h"
#include "math/math.h"
#include "asset/asset.h"

#define MAX_VERTEX_ATTRIBUTES 16

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
  
  // @Todo: Material
};

struct Triangle_Mesh 
{
  String            name;

  s64               num_vertices;
  Vertex           *vertices;

  s64               num_indices;
  u32              *indices;

  s64               num_parts;
  Mesh_Part        *parts;
};

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
  s32  parent; // Always precedes the child. -1 for the root
  m4x4 local_transform;
  m4x4 inverse_bind_matrix;
};

struct Skeleton 
{
  Joint *joints;
  s64    num_joints;
};




#endif // RTS_MESH_H
