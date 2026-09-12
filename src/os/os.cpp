// Copyright Seong Woo Lee. All Rights Reserved.

#include "os/os.h"
#include "basic/arena.h"
#include "basic/allocator.h"
#include "basic/string.h"

String read_entire_file(Arena* arena, String file_path) {
    String result = {};

    OS_Access_Flags flags = OS_ACCESS_FLAG_READ | OS_ACCESS_FLAG_SHARE_READ;
    OS_Handle file = os_open_file(file_path, flags);
    u64 file_size = os_get_file_size(file);
    u8 *ptr = push_array(arena, u8, file_size);
    u64 read_size = os_read_file(file, 0, file_size, ptr);
    assert(read_size == file_size);

    result.str = ptr;
    result.len = read_size;
    os_close_file(file);

    assert(result.str);
    return result;
}

String read_entire_file(String file_path, Allocator allocator) {
    String result = {};

    OS_Access_Flags flags = OS_ACCESS_FLAG_READ | OS_ACCESS_FLAG_SHARE_READ;
    OS_Handle file = os_open_file(file_path, flags);
    u64 file_size = os_get_file_size(file);
    u8 *ptr = (u8*)alloc(sizeof(u8) * file_size, allocator);
    u64 read_size = os_read_file(file, 0, file_size, ptr);
    assert(read_size == file_size);

    result.str = ptr;
    result.len = read_size;
    os_close_file(file);

    assert(result.str);
    return result;
}
