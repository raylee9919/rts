// Copyright Seong Woo Lee. All Rights Reserved.

void *alloc(u64 size, Allocator allocator) {
    auto a = (allocator.proc == NULL) ? tctx.allocator : allocator;
    return a.proc(ALLOCATOR_MODE_ALLOCATE, size, 0, NULL, a.data);
}

void *realloc(void *memory, u64 size, u64 old_size, Allocator allocator) {
    auto a = (allocator.proc == NULL) ? tctx.allocator : allocator;
    return a.proc(ALLOCATOR_MODE_RESIZE, size, old_size, memory, a.data);
}

void dealloc(void *memory, Allocator allocator) {
    auto a = (allocator.proc == NULL) ? tctx.allocator : allocator;
    a.proc(ALLOCATOR_MODE_FREE, 0, 0, memory, a.data);
}

void release(Allocator allocator) {
    allocator.proc(ALLOCATOR_MODE_RELEASE, 0, 0, NULL, allocator.data);
}
