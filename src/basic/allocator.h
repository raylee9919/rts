// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RTS_ALLOCATOR_H
#define RTS_ALLOCATOR_H

#include "basic/core.h"


enum Allocator_Mode {
    ALLOCATOR_MODE_ALLOCATE = 0,
    ALLOCATOR_MODE_RESIZE   = 1,
    ALLOCATOR_MODE_FREE     = 2,
    ALLOCATOR_MODE_RELEASE  = 3,
};

typedef void *Allocator_Procedure(Allocator_Mode mode, u64 size, u64 old_size,
                                  void *old_memory, void *allocator_data);

struct Allocator {
    Allocator_Procedure *proc = nullptr;
    void *data = nullptr;
};


//
// Each call passes corresponding mode to the allocating procedure.
//
u8   *alloc(u64 size, Allocator allocator);
void *realloc(void *memory, u64 size, u64 old_size, Allocator allocator);
void  dealloc(void *memory, Allocator allocator);
void  release(Allocator allocator);

#endif // RTS_ALLOCATOR_H
