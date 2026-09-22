// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RTS_MATERIAL_H
#define RTS_MATERIAL_H

#include "basic/string.h"
#include "basic/hash_table.h"
#include "rhi/rhi.h"
#include "generated/material.h"

struct Shader_Compiler;
struct Material_Field_Info;

#define MAX_CPU_MATERIAL_SIZE 1024 // @Temporary

struct Bitmap {
    RHI_Format      format;
    void            *data;
    u64             size;
    u32             width;
    u32             height;
};

struct Material_Entry {
    u64             offset;   // offset into material buffer
    Guid            pipeline;
    Guid            type_id;  // Key into `material_type_table`
    alignas(16) u8  data[MAX_CPU_MATERIAL_SIZE];
};

void material_type_system_init(Allocator allocator);

Material_Entry *material_alloc(Guid guid);
b32 material_dealloc(Guid guid);
Material_Entry *material_from_guid(Guid guid);

void image_load_proc( String filepath, String short_name, void *user_data );

void material_load_proc( String filepath, String short_name, void *user_data );


extern Table<Guid, Material_Entry, hash_guid> material_table;


#endif // RTS_MATERIALH
