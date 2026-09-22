// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RTS_MATERIAL_H
#define RTS_MATERIAL_H

#include "basic/string.h"
#include "basic/hash_table.h"
#include "rhi/rhi.h"

struct Shader_Compiler;
struct Material_Field_Info;

// ------------------------------------------------------------------------- //

#define MAX_MATERIAL_FIELDS     32
#define MAX_CPU_MATERIAL_SIZE  256

enum M_FieldType {
    M_FIELD_S8,
    M_FIELD_S16,
    M_FIELD_S32,
    M_FIELD_S64,

    M_FIELD_U8,
    M_FIELD_U16,
    M_FIELD_U32,
    M_FIELD_U64,

    M_FIELD_F32,
    M_FIELD_VEC2,
    M_FIELD_VEC3,
    M_FIELD_VEC4,

    M_FIELD_GUID,

    M_FIELD_TEXTURE, // bindless handle to texture

    M_FIELD_TYPE_COUNT,
};

struct M_Field {
    String      name;

    M_FieldType cpu_type;
    u64         cpu_size;
    u64         cpu_offset;

    M_FieldType gpu_type;
    u64         gpu_size;
    u64         gpu_offset;
};

struct M_TypeInfo {
    Guid    id; // hash from canonical path from data dir
    u32     num_fields;
    M_Field fields[MAX_MATERIAL_FIELDS];
    u64     cpu_size;
    u64     gpu_size;
};

struct M_Entry {
    Guid type_id; // key into `material_type_table`
    Guid pipeline;
    u64  offset;  // offset into gpu buffer
    alignas(16) u8 data[MAX_CPU_MATERIAL_SIZE];
};

b32 material_system_init(String material_shader_dir, Shader_Compiler *shader_compiler);
void material_system_shutdown();
Pair<b32, M_TypeInfo> get_material_type_info(Shader_Compiler *shader_compiler, String filepath);
void write_gpu_material(M_TypeInfo *info, void *gpu_ptr, void *material);
M_Entry *alloc_material(Guid id);
M_Entry *get_material(Guid id);

// ------------------------------------------------------------------------- //

struct Bitmap {
    RHI_Format      format;
    void            *data;
    u64             size;
    u32             width;
    u32             height;
};

void image_load_proc( String filepath, String short_name, void *user_data );

void texture_load_proc( String filepath, String short_name, void *user_data );

void material_load_proc( String filepath, String short_name, void *user_data );


#endif // RTS_MATERIALH
