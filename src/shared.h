// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RTS_SHARED_H
#define RTS_SHARED_H

#include "basic/string.h"
struct Arena;

struct Shared {
    /* Bundle things that ought to share the lifetime throughout the application */
    Arena *arena;
    
    /* Paths */
    String appdata_path;
    String data_path;
    String source_path;
};

extern Shared *shared;

void shared_init();


#endif // RTS_SHARED_H
