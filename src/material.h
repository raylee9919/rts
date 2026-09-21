// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RTS_MATERIAL_H
#define RTS_MATERIAL_H

#include "basic/string.h"
#include "rhi/rhi.h"

#include "generated/material.h"

struct Shader_Compiler;

struct Bitmap {
    RHI_Format      format;
    void            *data;
    u64             size;
    u32             width;
    u32             height;
};

// Generate codes for material system.
void material_codegen(Shader_Compiler *shader_compiler,
                      String material_shader_dir,
                      String output_dir);

void image_load_proc( String filepath, String short_name, void *user_data );

void material_load_proc( String filepath, String short_name, void *user_data );

#endif // RTS_MATERIALH
