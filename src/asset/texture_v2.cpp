// Copyright Seong Woo Lee. All Rights Reserved.

static Bitmap_Format bitmap_compute_format(int num_channels, b32 is_hdr, b32 is_16_bit) {
    if (is_hdr) {
        Assert(!"X"); // @Todo: HDR
    } else if (is_16_bit) {
        if (num_channels == 1) return BITMAP_FORMAT_R16_UNORM;
        if (num_channels == 2) return BITMAP_FORMAT_RG16_UNORM;
        if (num_channels == 4) return BITMAP_FORMAT_RGBA16_UNORM;
        else Assert(!"Unsupported 16-bit channel count");
    } else {
        if (num_channels == 1) return BITMAP_FORMAT_R8_UNORM;
        if (num_channels == 2) return BITMAP_FORMAT_RG8_UNORM;
        if (num_channels == 4) return BITMAP_FORMAT_RGBA8_UNORM;
        else Assert(!"Unsupported channel count");
    }
    return BITMAP_FORMAT_INVALID;
}

Bitmap bitmap_import(void *loaded_data, u64 size) {
    Bitmap result = {};

    u8 *data = (u8*)loaded_data;
    int sz = (int)size;

    int w, h, num_channels;

    stbi_info_from_memory(data, sz, &w, &h, &num_channels);
    
    int force_channel = num_channels == 3 ? 4 : num_channels;

    b32 is_hdr    = stbi_is_hdr_from_memory(data, sz);
    b32 is_16_bit = stbi_is_16_bit_from_memory(data, sz);

    if (is_hdr) {
        Assert(!"X"); // @Todo: HDR
    }

    void *ptr = NULL;
    if (is_16_bit) {
        ptr = stbi_load_16_from_memory(data, sz, &w, &h, &num_channels, force_channel);
    } else {
        ptr = stbi_load_from_memory(data, sz, &w, &h, &num_channels, force_channel);
    }

    if (ptr) {
        result.format = bitmap_compute_format(force_channel, is_hdr, is_16_bit);
        result.data   = ptr;
        result.size   = w * h * force_channel * (is_16_bit ? 2 : 1);
        result.width  = w;
        result.height = h;
    }

    return result;
}

void bitmap_free(Bitmap *bitmap) {
    stbi_image_free(bitmap->data);
}
