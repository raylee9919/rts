/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Seong Woo Lee $
   $Notice: (C) Copyright %s by Seong Woo Lee. All Rights Reserved. $
   ======================================================================== */


internal Utf8
read_entire_file(Arena *arena, Utf8 file_path)
{
    Utf8 result = {};

    Os_Handle file = os.file_open(file_path, OS_FILE_ACCESS_READ);
    mmm file_size = 0;
    if (os.handle_valid(file))
    {
        file_size = os.file_size(file);
        u8 *ptr = push_array(arena, u8, file_size);
        mmm read_size = os.file_read(file, ptr, file_size);
        Assert(read_size == file_size);

        result.str = ptr;
        result.len = read_size;
    }
    os.file_close(file);

    Assert(result.str);
    return result;
}
