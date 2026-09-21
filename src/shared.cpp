// Copyright Seong Woo Lee. All Rights Reserved.

#include "basic/arena.h"
#include "shared.h"
#include "shader_compiler/shader.h"

Shared *shared;

void shared_init() {
    /* Bootstrap arena */
    Allocator arena = arena_allocator_alloc();
    shared = (Shared*)alloc(sizeof(Shared), arena);
    shared->arena = arena;

    /* Debug */
#if BUILD_DEBUG
    shared->debug = true;
#else
    shared->debug = false;
#endif

    /* Gather paths */
    // @Todo: This is wrong. I need Path struct and String_Builder
#if 0
    String exe_path             = get_path_of_running_executable(tctx.temp);
    String exe_dir              = path_strip_filename(exe_path);
    String local_data_path      = tprint(S("%Scontents/"), exe_dir);
    s64 index                   = find_index_of_any_from_right(exe_dir, S("\\/"));
    String exe_parent_dir       = slice(exe_dir, 0, index + 1);
    String parent_data_path     = tprint(S("%Scontents/"), exe_parent_dir);

    if (directory_exists(local_data_path)) {
        shared->data_path = str_copy(local_data_path, arena); 
    } else if (directory_exists(parent_data_path)) {
        shared->data_path = str_copy(parent_data_path, arena); 
    }

    String data_parent_path = utf8_path_chop_last_slash(shared->data_path);
    String src_path = tprint(S("%S/src"), data_parent_path);
    if (directory_exists(src_path)) {
        shared->source_path.str = alloc(sizeof(u8) * src_path.len, arena);
        shared->source_path.len = src_path.len;
        memcpy(shared->source_path.str, src_path.str, src_path.len);
    }
#else
    shared->data_path   = S("C:/dev/rts/data/");
    shared->source_path = S("C:/dev/rts/src/");
#endif


    /* Shader Compiler */
    shared->shader_compiler = (Shader_Compiler*)alloc(sizeof(Shader_Compiler), shared->arena);
    Assert(shader_compiler_init(shared->shader_compiler));
    shared->shader_compiler->include_path = S("C:/dev/rts/data/shaders/"); // @Temporary
}

void shared_shutdown() {
    /* Shader Compiler */
    shader_compiler_shutdown(shared->shader_compiler);
}
