// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RTS_SHARED_H
#define RTS_SHARED_H

#include "basic/string.h"
#include "os/os.h"

struct Arena;
struct Shader_Compiler;

struct Shared {
    /* Bundle things that ought to share the lifetime throughout the application */
    Allocator arena;

    /* Window */
    OS_Handle window;
    
    /* Paths */
    String data_path;
    String source_path;

    /* Shader Compiler */
    Shader_Compiler *shader_compiler;
};

extern Shared *shared;

void shared_init();
void shared_shutdown();


#endif // RTS_SHARED_H
