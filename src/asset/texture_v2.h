// Copyright Seong Woo Lee. All Rights Reserved.

enum Bitmap_Format {
    BITMAP_FORMAT_INVALID = 0,

    BITMAP_FORMAT_R8_UNORM,
    BITMAP_FORMAT_RG8_UNORM,
    BITMAP_FORMAT_RGBA8_UNORM,

    BITMAP_FORMAT_R16_UNORM,
    BITMAP_FORMAT_RG16_UNORM,
    BITMAP_FORMAT_RGBA16_UNORM,

    BITMAP_FORMAT_R16F,
    BITMAP_FORMAT_RG16F,
    BITMAP_FORMAT_RGBA16F,

    BITMAP_FORMAT_R32F,
    BITMAP_FORMAT_RG32F,
    BITMAP_FORMAT_RGBA32F,
};

struct Bitmap {
    Bitmap_Format   format;
    void            *data;
    u64             size;
    u32             width;
    u32             height;
};

internal Bitmap bitmap_import(void *loaded_data, u64 size);
internal void   bitmap_free(Bitmap *bitmap);
