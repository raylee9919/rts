// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RTS_STRING_BUILDER_H
#define RTS_STRING_BUILDER_H

#include "basic/allocator.h"

#define STRING_BUILDER_HEADER_SIZE  sizeof(String_Builder::Header)
#define STRING_BUILDER_BUFFER_SIZE  4096 - STRING_BUILDER_HEADER_SIZE

struct String_Builder {
    /* |----Header----|------------Buffer------------| */
    struct Header {
        s64     count;
        s64     allocated;
        Header *next;
    };

    Allocator allocator;

    Header   *current_buffer;
    u8        initial_bytes[STRING_BUILDER_BUFFER_SIZE];
};

void init_string_builder(String_Builder *builder, Allocator allocator);


#endif // RTS_STRING_BUILDER_H
