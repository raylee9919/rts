/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Sung Woo Lee $
   $Notice: (C) Copyright %s by Sung Woo Lee. All Rights Reserved. $
   ======================================================================== */

#pragma pack(push, 1)

struct Bitmap {
    s32 bits_per_channel;
    s32 channel_count;
    s32 width;
    s32 height;
    s32 pitch;
    u32 handle;
    u32 size;
    void *memory;
};

enum Asset_ID {
    Asset_Invalid = 0,
};

enum Asset_State {
    Asset_State_Unloaded,
    Asset_State_Queued,
    Asset_State_Loaded
};

//
// Font
//
struct Asset_Font_Header {
    u32 kerning_pair_count;
    u32 vertical_advance;
    f32 ascent;
    f32 descent;
    f32 max_width;
};

struct Asset_Kerning {
    u32 first;
    u32 second;
    s32 value;
};

struct Asset_Glyph {
    u32 codepoint;
    s32 ascent;
    s32 A;
    s32 B;
    s32 C;
    Bitmap bitmap;
};


#pragma pack(pop)
