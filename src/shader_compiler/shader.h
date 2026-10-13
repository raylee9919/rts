// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RTS_SHADER_H
#define RTS_SHADER_H

#include "basic/core.h"
#include "basic/allocator.h"
#include "basic/string.h"

#include "./slang/slang.h"

struct Shader_Compiler;

enum Shader_Stage : u8 
{
  SHADER_STAGE_NULL = 0,
  SHADER_STAGE_VS,        // Vertex shader
  SHADER_STAGE_PS,        // Pixel (fragment) shader
  SHADER_STAGE_CS,        // Compute shader
  SHADER_STAGE_MS,        // Mesh shader
  SHADER_STAGE_TS,        // Task (amplification) shader
};

struct Shader_Compile_Options 
{
  Shader_Stage stage;
  String       source;
  String       path;    // Used for diagnostics. Optional.

  // Material implementation linked in to resolve `extern struct Material`. Optional.
  String       material_source;
  String       material_path;
};

struct Shader_Compile_Result 
{
  Shader_Stage stage;
  u8          *data;
  u64          size;
};

enum Shader_Field_Type : u32 
{
  SHADER_FIELD_INVALID = 0,

  SHADER_FIELD_FLOAT,
  SHADER_FIELD_FLOAT2,
  SHADER_FIELD_FLOAT3,
  SHADER_FIELD_FLOAT4,

  SHADER_FIELD_INT8,
  SHADER_FIELD_INT16,
  SHADER_FIELD_INT32,
  SHADER_FIELD_INT64,

  SHADER_FIELD_UINT8,
  SHADER_FIELD_UINT16,
  SHADER_FIELD_UINT32,
  SHADER_FIELD_UINT64,
};

struct Shader_Field 
{
  String            attribute; // e.g.  [texture]
  Shader_Field_Type type;      // e.g.  uint32_t
  String            name;      // e.g.  "albedo_texture_id"
};

struct Shader_Struct 
{
  String            name;
  Shader_Field      fields[256];
  u32               num_fields;
};


/* Initialize shader compiler */
b32 
shader_compiler_init(Shader_Compiler *compiler);

/* Deinitialize shader compiler */
void 
shader_compiler_shutdown(Shader_Compiler *compiler);


/*
  Compiles to shader binary.
  `allocator` allocates memory for it.
*/
b32 
shader_compile(Shader_Compiler       *compiler,
               Shader_Compile_Options options,
               bool                   debug,
               Shader_Compile_Result *out_result,
               Allocator              allocator);


/*
  Reflects material shader source file and 
  returns fields of the implemented `IMaterial`.
*/
Pair<b32, Shader_Struct> 
shader_reflect_material(Shader_Compiler *compiler, 
                        String           filepath); 


/*
  Converts `Shader_Field_Type` to string.
  For example, `SHADER_FIELD_FLOAT` becomes `float`.
*/
String 
string_from_shader_field_type(Shader_Field_Type type);

#endif // RTS_SHADER_H
