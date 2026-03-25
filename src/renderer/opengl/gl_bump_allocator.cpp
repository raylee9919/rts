// Copyright Seong Woo Lee. All Rights Reserved.

namespace GL
{
    void bump_init(Bump_Allocator *b, u64 size)
    {
        b->size = size;
        b->used = 0;
        glCreateBuffers(1, &b->handle);
        glNamedBufferStorage(b->handle, size, NULL, GL_DYNAMIC_STORAGE_BIT);
    }

    void bump_push(Bump_Allocator *b, u64 size)
    {
        assert(b->used + size < b->size);

        b->used += size;
    }
}
