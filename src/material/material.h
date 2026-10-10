// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RTS_MATERIAL_H
#define RTS_MATERIAL_H

#include "basic/string.h"
#include "rhi/rhi.h"
#include "asset/asset.h"

struct Shader_Compiler;
struct Material_Field_Info;

// ------------------------------------------------------------------------- //

#define MAX_MATERIAL_FIELDS     32
#define MAX_CPU_MATERIAL_SIZE  256

const read_only String DEFAULT_MATERIAL_SHADER_PATH = S("shaders/material/default.slang");

enum M_FieldType 
{
  MTL_FIELD_S8,
  MTL_FIELD_S16,
  MTL_FIELD_S32,
  MTL_FIELD_S64,

  MTL_FIELD_U8,
  MTL_FIELD_U16,
  MTL_FIELD_U32,
  MTL_FIELD_U64,

  MTL_FIELD_F32,
  MTL_FIELD_VEC2,
  MTL_FIELD_VEC3,
  MTL_FIELD_VEC4,

  MTL_FIELD_GUID,

  MTL_FIELD_TEXTURE, // bindless handle to texture

  MTL_FIELD_TYPE_COUNT,
};

struct M_Field 
{
  String      name;

  M_FieldType cpu_type;
  u64         cpu_size;
  u64         cpu_offset;

  M_FieldType gpu_type;
  u64         gpu_size;
  u64         gpu_offset;
};

struct M_TypeInfo 
{
  Guid    id; // hash from canonical path from data dir
  u32     num_fields;
  M_Field fields[MAX_MATERIAL_FIELDS];
  u64     cpu_size;
  u64     gpu_size;
};

struct M_Entry 
{
  Guid type_id; // key into `material_type_table`
  Guid pipeline;
  u64  offset;  // offset into gpu buffer
  alignas(16) u8 data[MAX_CPU_MATERIAL_SIZE];
};

b32 
material_system_init(String material_shader_dir, 
                     Shader_Compiler *shader_compiler);

void 
material_system_shutdown();

Pair<b32, M_TypeInfo> 
get_material_type_info(Shader_Compiler *shader_compiler, String filepath);

void 
write_gpu_material(M_TypeInfo *info, void *gpu_ptr, void *material);

M_Entry *
alloc_material(Guid id);

M_Entry *
material_from_guid(Guid id);


// ------------------------------------------------------------------------- //

struct Bitmap {
    RHI_Format      format;
    void            *data;
    u64             size;
    u32             width;
    u32             height;
};

ASSET_LOAD_PROC(image_load_proc);
ASSET_LOAD_PROC(texture_load_proc);
ASSET_LOAD_PROC(material_load_proc);


#endif // RTS_MATERIALH
