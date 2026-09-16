// Copyright Seong Woo Lee. All Rights Reserved.

#include "basic/string_builder.h"

void init_string_builder(String_Builder *builder, Allocator allocator) {
    builder->allocator = allocator;
}
