// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RTS_ALLOCATOR_H
#define RTS_ALLOCATOR_H


enum Allocator_Mode {
    ALLOCATOR_MODE_ALLOCATE = 0,
    ALLOCATOR_MODE_RESIZE   = 1,
    ALLOCATOR_MODE_FREE     = 2,
    ALLOCATOR_MODE_RELEASE  = 3,
};

typedef void *Allocator_Procedure(Allocator_Mode mode, u64 size, u64 old_size,
                                  void *old_memory, void *allocator_data);

struct Allocator {
    Allocator_Procedure *proc;
    void *data;
};


//
// Each call passes corresponding mode to the allocating procedure.
//
#define alloc_t(T, ...)     (T *)alloc(sizeof(T), ##__VA_ARGS__)
static void *alloc(u64 size, Allocator allocator = {});
static void *realloc(void *memory, u64 size, u64 old_size, Allocator allocator = {});
static void  dealloc(void *memory, Allocator allocator = {});
static void  release(Allocator allocator);

#endif // RTS_ALLOCATOR_H
