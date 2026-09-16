// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RTS_SHADER_DXC_H
#define RTS_SHADER_DXC_H

#include <windows.h>
#include "third_party/DXC/Include/dxcapi.h"

#include "basic/string.h"


struct Shader_Compiler {
    IDxcCompiler3      *compiler_3;
    IDxcUtils          *utils;
    IDxcIncludeHandler *include_handler;

    String              include_path;
};


#endif // RTS_SHADER_DXC_H
