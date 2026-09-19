// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RTS_SHADER_SLANG_H
#define RTS_SHADER_SLANG_H

#include "third_party/slang/include/slang.h"
#include "basic/string.h"


struct Shader_Compiler {
    slang::IGlobalSession *global_session;

    String                 include_path;
};

#endif // RTS_SHADER_SLANG_H
