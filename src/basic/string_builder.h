// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RTS_STRING_BUILDER_H
#define RTS_STRING_BUILDER_H

#include "basic/allocator.h"
#include "basic/string.h"

#define STRING_BUILDER_BUFFER_SIZE     4096

struct String_Builder {
    struct Buffer {
        s64     count;
        s64     allocated;
        Buffer *next;
    };

    Allocator allocator;

    s64       subsequent_buffer_size;

    Buffer   *current_buffer;
    u8        initial_bytes[sizeof(String_Builder::Buffer) + STRING_BUILDER_BUFFER_SIZE];
};

void    init(String_Builder *builder, Allocator allocator, s64 buffer_size = -1);

void    append(String_Builder *builder, u8 *str, s64 size);

void    append(String_Builder *builder, String str);

void    append(String_Builder *builder, u8 byte);

void    append(String_Builder *builder, const char *cstr);

s64     string_length(String_Builder *builder);

void    reset(String_Builder *builder);

String  flush(String_Builder *builder, bool do_reset = true);


#endif // RTS_STRING_BUILDER_H
