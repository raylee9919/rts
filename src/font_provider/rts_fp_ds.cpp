internal void
_arrsetlen(Arena *arena, void **arr, u64 size, u64 count)
{
    if (*arr == 0)
    {
        u8 *ptr = (u8 *)push_size(arena, sizeof(Array_Header) + size*count);
        Array_Header *header = (Array_Header *)ptr;
        header->len  = count;
        header->cap = count;
        u8 *base = ptr + sizeof(Array_Header);
        *arr = base;
    }
    else
    {
        Array_Header *header = (Array_Header *)((u8 *)(*arr) - sizeof(Array_Header));
        if (header->cap >= count)
        {
            header->cap = count;
        }
        else if (header->len >= count)
        {
            header->cap = count;
        }
        else
        {
            u64 new_len = (count*2);
            u8 *ptr = (u8 *)push_size(arena, sizeof(Array_Header) + size*new_len);

            u8 *new_base = ptr + sizeof(Array_Header);
            memory_copy(new_base, *arr, size*header->cap);

            Array_Header *new_header = (Array_Header *)ptr;
            new_header->len  = new_len;
            new_header->cap = count;

            *arr = new_base;
        }
    }
}

internal u64
_arrlenu(void **arr)
{
    u64 result = 0;
    if (*arr != 0)
    {
        Array_Header *header = (Array_Header *)((u8 *)(*arr) - sizeof(Array_Header));
        result = header->cap;
    }
    return result;
}

