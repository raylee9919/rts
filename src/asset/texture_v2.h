// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RTS_ASSET_TEXTURE_V2_H
#define RTS_ASSET_TEXTURE_V2_H

#include "basic/core.h"

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

Bitmap bitmap_import(void *loaded_data, u64 size);
void   bitmap_free(Bitmap *bitmap);

#endif // RTS_ASSET_TEXTURE_V2_H
