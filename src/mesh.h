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


#define VERTEX_ATTRIBUTE_POSITION  S("position")
#define VERTEX_ATTRIBUTE_UV        S("uv")
#define VERTEX_ATTRIBUTE_NORMAL    S("normal")
#define VERTEX_ATTRIBUTE_TANGENT   S("tangent")
#define VERTEX_ATTRIBUTE_JOINT     S("joint")
#define VERTEX_ATTRIBUTE_WEIGHT    S("weight")


enum Vertex_Attribute_Type : s16
{
  VERTEX_ATTRIBUTE_INVALID = 0,
  VERTEX_ATTRIBUTE_FLOAT32,
  VERTEX_ATTRIBUTE_INT32,
};

struct Vertex_Attribute
{
  // semantically binds with the engine.
  // List: 
  // "position", "uv", "normal", "tangent", "joint", "weight"
  String                name; 

  Vertex_Attribute_Type type;
  s64                   width;
  s64                   stride; // equivalent to `type.size * width`
  s64                   offset; // offset in a single vertex
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
  String                  name;

  Array<Vertex_Attribute> attributes;

  s64                     num_vertices;
  u8                     *vertices;
  s64                     vertex_size;

  s64                     num_indices;
  u32                    *indices;

  s64                     num_parts;
  Mesh_Part              *parts;

  AABB                    aabb;
};

s64
vertex_size_from_attributes(Array<Vertex_Attribute> *attributes);

b32 
mesh_has_attribute(Triangle_Mesh *mesh, String attribute_name);

b32 
mesh_is_skeletal(Triangle_Mesh *mesh);

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

Pair<Skeleton, b32>
skeleton_from_guid(Guid id);


#endif // RTS_MESH_H
