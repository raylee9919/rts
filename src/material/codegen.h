// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RTS_MATERIAL_CODEGEN_H
#define RTS_MATERIAL_CODEGEN_H

#include "basic/string.h"

struct Shader_Compiler;

// Generates data/generated/material.h from the material shaders in `material_shader_dir`.
void material_codegen(Shader_Compiler *shader_compiler,
                      String material_shader_dir,
                      String output_dir);

#endif // RTS_MATERIAL_CODEGEN_H
