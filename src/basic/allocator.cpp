// Copyright Seong Woo Lee. All Rights Reserved.

void* allocate(Allocator* allocator, u64 size) {
    return allocator->proc(ALLOCATOR_MODE_ALLOCATE, size, 0, nullptr, allocator->data);
}

void deallocate(Allocator* allocator) {
    allocator->proc(ALLOCATOR_MODE_FREE, 0, 0, nullptr, allocator->data);
}
