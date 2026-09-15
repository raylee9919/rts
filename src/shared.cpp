// Copyright Seong Woo Lee. All Rights Reserved.

#include "basic/arena.h"
#include "os/os.h"
#include "shared.h"

Shared *shared;

void shared_init() {
    /* Bootstrap arena */
    Arena *arena = arena_alloc();
    shared = push_struct(arena, Shared);
    shared->arena = arena;

    /* Gather paths */
    shared->appdata_path = os->appdata_path;

    String local_data_path    = tprint(S("%S/contents"), os->binary_path);
    String binary_parent_path = utf8_path_chop_last_slash(os->binary_path);
    String parent_data_path   = tprint(S("%S/contents"), binary_parent_path);

    if (directory_exists(local_data_path)) {
        shared->data_path = utf8_copy(arena, local_data_path); 
    } else if (directory_exists(parent_data_path)) {
        shared->data_path = utf8_copy(arena, parent_data_path); 
    }

    String data_parent_path = utf8_path_chop_last_slash(shared->data_path);
    String src_path = tprint(S("%S/src"), data_parent_path);
    if (directory_exists(src_path)) {
        shared->source_path.str = push_array(arena, u8, src_path.len);
        shared->source_path.len = src_path.len;
        memcpy(shared->source_path.str, src_path.str, src_path.size());
    }
}
