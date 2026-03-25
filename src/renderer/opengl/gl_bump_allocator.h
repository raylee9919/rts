// Copyright Seong Woo Lee. All Rights Reserved.

#pragma once

namespace GL
{
    struct Bump_Allocator
    {
        GLuint  handle;
        u64     size;
        u64     used;
    };

    void bump_init(Bump_Allocator *b, u64 size);

    void bump_push(Bump_Allocator *b, u64 size);
}
