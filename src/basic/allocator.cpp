// Copyright Seong Woo Lee. All Rights Reserved.

#include "basic/allocator.h"

u8 *alloc(u64 size, Allocator allocator) {
    return (u8 *)allocator.proc(ALLOCATOR_MODE_ALLOCATE, size, 0, NULL, allocator.data);
}

void *realloc(void *memory, u64 size, u64 old_size, Allocator allocator) {
    return allocator.proc(ALLOCATOR_MODE_RESIZE, size, old_size, memory, allocator.data);
}

void dealloc(void *memory, Allocator allocator) {
    allocator.proc(ALLOCATOR_MODE_FREE, 0, 0, memory, allocator.data);
}

void destroy(Allocator allocator) {
    allocator.proc(ALLOCATOR_MODE_DESTROY, 0, 0, NULL, allocator.data);
}

void clear(Allocator allocator) {
    allocator.proc(ALLOCATOR_MODE_CLEAR, 0, 0, NULL, allocator.data);
}
