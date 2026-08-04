// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RTS_SHADER_DXC_H
#define RTS_SHADER_DXC_H


struct Shader_Compiler {
    IDxcCompiler3      *compiler_3;
    IDxcUtils          *utils;
    IDxcIncludeHandler *include_handler;
};


#endif // RTS_SHADER_DXC_H
