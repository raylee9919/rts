// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RTS_IMPORTER_FBX_H
#define RTS_IMPORTER_FBX_H

#include "basic/string.h"

struct FBX_Import_Options
{
  // If false, use ufbx as default
  b8 generate_normals_with_mesh_optimizer;
  b8 overwrite_existing_file;
};

b32
fbx_import(String             filepath, 
           String             out_directory,
           FBX_Import_Options options);

#endif // RTS_IMPORTER_FBX_H
