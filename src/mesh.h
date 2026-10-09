// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RTS_MESH_H
#define RTS_MESH_H

#include "basic/array.h"
#include "basic/string.h"
#include "os/os.h"
#include "math/math.h"
#include "asset.h"

//
// M e s h
//
enum Vertex_Attribute_Type : u16
{
  VERTEX_ATTRIBUTE_FLOAT32,
  VERTEX_ATTRIBUTE_INT32,
};

struct Vertex_Attribute 
{
  // Name of the vertex attribute (e.g. "normal")
  String name;

  // Data type and array length. 
  // (e.g. bone weight might be type: float32 with width: 4)
  Vertex_Attribute_Type type;
  s64 width;

  // Offset of this attribute in a vertex.
  s64 offset;
};

struct Mesh_Part
{
  // offset and count into index buffer
  s64 offset;
  s64 count;
};

struct Triangle_Mesh 
{
  String            name;

  s64               num_attributes;
  Vertex_Attribute  attributes[16];

  s64               num_vertices;
  u8               *vertices;

  s64               num_indices;
  u32              *indices;

  s64               num_parts;
  Mesh_Part        *parts;
};

String
string_from_mesh(Triangle_Mesh *mesh, Allocator allocator);


//
// S k e l e t o n
//
struct Joint 
{
  Name name;
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
