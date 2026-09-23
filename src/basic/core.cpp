// Copyright Seong Woo Lee. All Rights Reserved.

#include "basic/core.h"
#include "basic/allocator.h"
#include "basic/context.h"
#include "profiler/profiler.h"

u16
to_u16_safe(u32 x)
{
    R_ASSERT(x <= U16_MAX);
    u16 result = (u16)x;
    return result;
}

u32
to_u32_safe(u64 x)
{
    R_ASSERT(x <= U32_MAX);
    u32 result = (u32)x;
    return result;
}

s32
to_s32_safe(s64 x)
{
    R_ASSERT(x <= S32_MAX);
    s32 result = (s32)x;
    return result;
}

void *
_dll_np(void *node, u64 np)
{
    void *result = ptr_from_int(*(u64 *)((u8 *)node + np));
    return result;
}

void
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
                memcpy(tmp1, it, size);
                memcpy(tmp2, in, size);
                memcpy(it, tmp2, size);
                memcpy(in, tmp1, size);
                u8 *it8 = (u8 *)it;
                u8 *in8 = (u8 *)in;
                memcpy(it8 + next, tmp1 + next, sizeof(void *));
                memcpy(in8 + next, tmp2 + next, sizeof(void *));
                memcpy(it8 + prev, tmp1 + prev, sizeof(void *));
                memcpy(in8 + prev, tmp2 + prev, sizeof(void *));
            }
        }
    }
}

void radix_sort_u64(void *data, s64 count, s64 stride, s64 key_offset)
{
    ProfileScope;
    void *tmp = alloc(count * stride, tctx.temp);

    for (int k = 0; k < 64; k += 8)
    {
        u32 n[256] = {0};
        u32 idx[256] = {0};

        for (s64 i = 0; i < count; ++i)
        {
            u64 key = *(u64*)((u8*)data + stride * i + key_offset);
            u8 b = (u8)(key >> k);
            n[b] += 1;
        }

        for (s64 i = 0; i < 255; ++i)
        {
            idx[i + 1] = idx[i] + n[i];
        }

        for (s64 i = 0; i < count; ++i)
        {
            u64 key = *(u64*)((u8*)data + stride * i + key_offset);
            u8 b = (u8)(key >> k);
            memcpy((u8*)tmp + idx[b] * stride, (u8*)data + i * stride, stride);
            idx[b] += 1;
        }

        void *swap = data;
        data = tmp;
        tmp = swap;
    }
}
