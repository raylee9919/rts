// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RTS_ALLOCATOR_H
#define RTS_ALLOCATOR_H


enum Allocator_Mode {
    ALLOCATOR_MODE_ALLOCATE = 0,
    ALLOCATOR_MODE_RESIZE   = 1,
    ALLOCATOR_MODE_FREE     = 2,
};

typedef void* Allocator_Procedure(Allocator_Mode mode, u64 size, u64 old_size, void* old_memory, void* allocator_data);

struct Allocator {
    Allocator_Procedure* proc;
    void* data;
};

internal void* alloc(u64 size);
internal void* realloc(void* memory, u64 size, u64 old_size);
internal void  Free(void* memory);


#endif // RTS_ALLOCATOR_H
