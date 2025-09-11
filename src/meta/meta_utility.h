/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Sung Woo Lee $
   $Notice: (C) Copyright 2025 by Sung Woo Lee. All Rights Reserved. $
   ======================================================================== */



    

internal Buffer
read_entire_file(const char *filepath) 
{
    Buffer result = {};
    FILE *file = fopen(filepath, "rb");
    if (file) 
    {
        fseek(file, 0, SEEK_END);
        umm filesize = ftell(file);
        fseek(file, 0, SEEK_SET);
        result.data = (u8 *)malloc(filesize);
        result.count = filesize;
        Assert(fread(result.data, filesize, 1, file) == 1);
        fclose(file);
    }
    else 
    {
        printf("[ERROR] Couldn't open file %s.\n", filepath);
    }
    return result;
}
