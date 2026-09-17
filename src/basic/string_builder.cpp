// Copyright Seong Woo Lee. All Rights Reserved.

#include "basic/string_builder.h"

static String_Builder::Buffer *get_base_buffer(String_Builder *builder) {
    return (String_Builder::Buffer*)builder->initial_bytes;
}

static u8 *get_buffer_data(String_Builder::Buffer *buffer) {
    return (u8*)buffer + sizeof(String_Builder::Buffer);
}

static bool expand(String_Builder *builder) {
    Assert(builder->allocator.proc != nullptr);
    
    s64 subsequent = (builder->subsequent_buffer_size > 0) ? builder->subsequent_buffer_size : STRING_BUILDER_BUFFER_SIZE;

    u8 *bytes = alloc(sizeof(String_Builder::Buffer) + subsequent, builder->allocator);
    if (!bytes)  return false;

    auto *buffer = (String_Builder::Buffer*)bytes;

    buffer->next      = nullptr;
    buffer->count     = 0;
    buffer->allocated = subsequent;

    auto *old_buffer = builder->current_buffer;
    old_buffer->next = buffer;

    builder->current_buffer = buffer;

    return true;
}

void init(String_Builder *builder, Allocator allocator, s64 buffer_size) {
    builder->allocator = allocator;

    builder->subsequent_buffer_size = (buffer_size > 0) ? buffer_size : STRING_BUILDER_BUFFER_SIZE;

    builder->current_buffer            = get_base_buffer(builder);
    builder->current_buffer->count     = 0;
    builder->current_buffer->allocated = STRING_BUILDER_BUFFER_SIZE;
    builder->current_buffer->next      = nullptr;
}

void append(String_Builder *builder, u8 *str, s64 size) {
    while (size > 0) {
        auto *buffer = builder->current_buffer;

        s64 space = buffer->allocated - buffer->count;
        if (space <= 0) {
            bool success = expand(builder);
            if (!success) {
                // @Robustness: Error-handling
            }

            // Grab new one:
            buffer = builder->current_buffer;
            Assert(buffer != nullptr);

            space = buffer->allocated - buffer->count;
            Assert(space > 0);
        }

        s64 to_copy = min(size, space);
        if (size > 0)  Assert(to_copy >= 0);

        memcpy(get_buffer_data(buffer) + buffer->count, str, to_copy);

        buffer->count += to_copy;

        str  += to_copy;
        size -= to_copy;
    }
}

void append(String_Builder *builder, String str) {
    append(builder, str.str, str.len);
}

void append(String_Builder *builder, u8 byte) {
    append(builder, &byte, 1);
}

s64 string_length(String_Builder *builder) {
    auto *buffer = get_base_buffer(builder);
    s64 bytes = 0;
    while (buffer) {
        bytes += buffer->count;
        buffer = buffer->next;
    }
    return bytes;
}

void reset(String_Builder *builder) {
    auto *base = get_base_buffer(builder);
    auto *buffer = base->next;
    while (buffer) {
        auto *next = buffer->next;
        dealloc(buffer, builder->allocator);
        buffer = next;
    }

    base->count = 0;
    base->next  = nullptr;

    builder->current_buffer = base;
}

String flush(String_Builder *builder, bool do_reset) {
    s64 count = string_length(builder);
    if (!count)  return {};

    String result;
    result.str = alloc(count, builder->allocator);
    result.len = count;

    u8 *dst = result.str;
    String_Builder::Buffer *buffer = get_base_buffer(builder);
    while (buffer) {
        memcpy(dst, get_buffer_data(buffer), buffer->count);
        dst += buffer->count;

        buffer = buffer->next;
    }

    if (do_reset) reset(builder);

    return result;
}
