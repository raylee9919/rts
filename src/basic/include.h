// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RTS_BASIC_INCLUDES_H
#define RTS_BASIC_INCLUDES_H

#include "./core.h"
#include "./arena.h"
#include "./allocator.h"
#include "./context.h"
#include "./string.h"
#include "./log.h"
#include "./array.h"
#include "./hash.h"
#include "./hash_table.h"


String read_entire_file(Arena* arena, String file_path);


#endif // RTS_BASIC_INCLUDES_H
