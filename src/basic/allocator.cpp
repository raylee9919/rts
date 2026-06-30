// Copyright Seong Woo Lee. All Rights Reserved.

void* alloc(u64 size) {
    auto a = tctx.allocator;
    return a.proc(ALLOCATOR_MODE_ALLOCATE, size, 0, nullptr, a.data);
}

void deallocate(Allocator* allocator) {
    allocator->proc(ALLOCATOR_MODE_FREE, 0, 0, nullptr, allocator->data);
}

void* realloc(void* memory, u64 size, u64 old_size) {
    auto a = tctx.allocator;
    return a.proc(ALLOCATOR_MODE_RESIZE, size, old_size, memory, a.data);
}

void Free(void* memory) {
    auto a = tctx.allocator;
    a.proc(ALLOCATOR_MODE_FREE, 0, 0, memory, a.data);
}
