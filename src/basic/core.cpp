// Copyright Seong Woo Lee. All Rights Reserved.

internal u16
to_u16_safe(u32 x)
{
    Assert(x <= U16_MAX);
    u16 result = (u16)x;
    return result;
}

internal u32
to_u32_safe(u64 x)
{
    Assert(x <= U32_MAX);
    u32 result = (u32)x;
    return result;
}

internal s32
to_s32_safe(s64 x)
{
    Assert(x <= S32_MAX);
    s32 result = (s32)x;
    return result;
}

internal void *
_dll_np(void *node, u64 np)
{
    void *result = ptr_from_int(*(u64 *)((u8 *)node + np));
    return result;
}

internal void
_dll_sort(void *first, void *last, u64 size, u64 next, u64 prev, int(*cmp)(void*,void*))
{
    for (void *end = last; end != first; end = _dll_np(end, prev))
    {
        for (void *it = first; it != end; it = _dll_np(it, next))
        {
            void *in = _dll_np(it, next);
            if (cmp(it, in))
            {
                u8 *tmp1 = (u8 *)alloc(size, tctx.temp);
                u8 *tmp2 = (u8 *)alloc(size, tctx.temp);
                memory_copy(tmp1, it, size);
                memory_copy(tmp2, in, size);
                memory_copy(it, tmp2, size);
                memory_copy(in, tmp1, size);
                u8 *it8 = (u8 *)it;
                u8 *in8 = (u8 *)in;
                memory_copy(it8 + next, tmp1 + next, sizeof(void *));
                memory_copy(in8 + next, tmp2 + next, sizeof(void *));
                memory_copy(it8 + prev, tmp1 + prev, sizeof(void *));
                memory_copy(in8 + prev, tmp2 + prev, sizeof(void *));
            }
        }
    }
}

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
