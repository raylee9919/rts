/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Sung Woo Lee $
   $Notice: (C) Copyright 2024 by Sung Woo Lee. All Rights Reserved. $
   ======================================================================== */


// --------------------------------
// @Note: [.cpp]
#include "base/rts_base_inc.h"
#include "os/rts_os.h"
#include "rts_math.h"
#include "rts_asset.h"

// --------------------------------
// @Note: [.cpp]
#include "base/rts_base_inc.cpp"
#include "os/rts_os.cpp"



// --------------------------------
// @Note: Defines.
#define ALLOC_SIZE GB(1)
#define BITMAP_WIDTH  1024
#define BITMAP_HEIGHT 1024


global HDC hdc;
global HBITMAP bitmap;
global void *bits;
global Arena *main_arena;


#define FONT_INPUT_DIRECTORY "../data/input/font/"
#define FONT_OUTPUT_DIRECTORY "../data/font/"

internal void
print_and_exit(const char *str) 
{
    fprintf(stderr, "%s\n", str);
    exit(1);
}

// ----------------------------------
// @Note: Your main concern.
internal Asset_Glyph *
bake_glyph(u32 codepoint, TEXTMETRIC metric) 
{
    Asset_Glyph *result = push_struct(main_arena, Asset_Glyph);
    wchar_t utf_codepoint = (wchar_t)codepoint;
    SIZE size;
    GetTextExtentPoint32W(hdc, &utf_codepoint, 1, &size);

    s32 width = size.cx;
    s32 height = size.cy;

    SetTextColor(hdc, RGB(0xff, 0xff, 0xff));
    TextOutW(hdc, 0, 0, &utf_codepoint, 1);

    s32 min_x = width - 1;
    s32 min_y = height - 1;
    s32 max_x = 0;
    s32 max_y = 0;

    u32 *pixel_row = (u32 *)bits + (BITMAP_HEIGHT - 1) * BITMAP_WIDTH;
    for (s32 y = 0;
            y < height;
            ++y) 
    {
        u32 *pixel = pixel_row;
        for (s32 x = 0;
                x < width;
                ++x) 
        {
            // COLORREF pixel = GetPixel(hdc, x, y); // OS-call, but suck it.
            u8 gray = (u8)(*pixel++ & 0xff);
            if (gray != 0) 
            {
                if (x < min_x) { min_x = x; }
                if (y < min_y) { min_y = y; }
                if (x > max_x) { max_x = x; }
                if (y > max_y) { max_y = y; }
            }
        }
        pixel_row -= BITMAP_WIDTH;
    }

    s32 baseline = metric.tmAscent;
    s32 ascent = baseline - min_y + 1;

    s32 off_x = min_x;
    s32 off_y = min_y;

    s32 glyph_width  = max_x - min_x + 1;
    s32 glyph_height = max_y - min_y + 1;
    s32 glyph_pitch  = glyph_width * 4;

    s32 margin = 1;

    result->codepoint = codepoint;
    result->ascent = ascent + margin;
    result->bitmap.bits_per_channel = 8;
    result->bitmap.channel_count = 4;
    result->bitmap.width = glyph_width + margin * 2;
    result->bitmap.height = glyph_height + margin * 2;
    result->bitmap.pitch = result->bitmap.width * 4;
    result->bitmap.handle = 0;
    result->bitmap.size = result->bitmap.height * result->bitmap.width * 4;
    result->bitmap.memory = push_size(main_arena, result->bitmap.size);

    u8 *dst_row = (u8 *)result->bitmap.memory + (result->bitmap.height - 1 - margin) * result->bitmap.pitch + 4 * margin;
    pixel_row = (u32 *)bits + (BITMAP_HEIGHT - 1 - off_y) * BITMAP_WIDTH;
    for (s32 y = 0;
            y < glyph_height;
            ++y) 
    {
        u32 *dst_at = (u32 *)dst_row;
        u32 *pixel = pixel_row + off_x;
        for (s32 x = 0;
                x < glyph_width;
                ++x) 
        {
            u8 gray = (u8)(*pixel++ & 0xff);
            u32 c = (gray << 24) |
                    (gray << 16) |
                    (gray <<  8) |
                     gray;
            *dst_at++ = c; 
        }
        dst_row -= result->bitmap.pitch;
        pixel_row -= BITMAP_WIDTH;
    }
    

    return result;
}

internal void
bake_font(const char *filename, const char *fontname, FILE* out, s32 cheese_height) 
{
    TEXTMETRIC metric = {};
    ABC *ABCs = 0;
    u32 lo = ' ';
    u32 hi = '~';

    if (AddFontResourceExA(filename, FR_PRIVATE, 0)) 
    {
        int weight = FW_THIN;
        HFONT font = CreateFontA(cheese_height, 0, 0, 0,
                                 weight, FALSE, FALSE, FALSE,
                                 DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                                 CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY,
                                 DEFAULT_PITCH|FF_DONTCARE, fontname);
        if (font) 
        { 
            HGDIOBJ prev_font = SelectObject(hdc, font);

            SetBkColor(hdc, RGB(0, 0, 0));

            GetTextMetrics(hdc, &metric);

            // Kerning.
            s32 kern_count = GetKerningPairsA(hdc, 0, 0);
            KERNINGPAIR *kern_pairs = push_array(main_arena, KERNINGPAIR, kern_count);
            GetKerningPairsA(hdc, kern_count, kern_pairs);

            // Font header.
            Asset_Font_Header font_header = {};
            font_header.kerning_pair_count = kern_count;
            font_header.vertical_advance = (metric.tmHeight + metric.tmInternalLeading + metric.tmExternalLeading);
            font_header.ascent = (f32)metric.tmAscent;
            font_header.descent = (f32)metric.tmDescent;
            font_header.max_width = (f32)metric.tmMaxCharWidth;

            Asset_Kerning *asset_kern_pairs = push_array(main_arena, Asset_Kerning, kern_count);
            for (s32 idx = 0; idx < kern_count; ++idx) 
            {
                KERNINGPAIR *kern_pair = kern_pairs + idx;
                Asset_Kerning *asset_kern_pair = asset_kern_pairs + idx;
                asset_kern_pair->first = kern_pair->wFirst;
                asset_kern_pair->second = kern_pair->wSecond;
                asset_kern_pair->value = kern_pair->iKernAmount;
            }

            // ABC. What a convenient name.
            ABCs = push_array(main_arena, ABC, hi - lo + 1);
            GetCharABCWidthsA(hdc, lo, hi, ABCs);

            // write font header.
            fwrite(&font_header, sizeof(font_header), 1, out);

            // write kerning pairs.
            fwrite(asset_kern_pairs, sizeof(*asset_kern_pairs) * kern_count, 1, out);

            // write glyphs.
            for (u32 ch = lo; ch <= hi; ++ch) 
            {
                Asset_Glyph *glyph = bake_glyph(ch, metric);
                glyph->A = ABCs[ch - lo].abcA;
                glyph->B = ABCs[ch - lo].abcB;
                glyph->C = ABCs[ch - lo].abcC;
                fwrite(glyph, sizeof(*glyph), 1, out);
                fwrite(glyph->bitmap.memory, glyph->bitmap.size, 1, out);
            }

            SelectObject(hdc, prev_font);
            DeleteObject(font);
        } else {
            print_and_exit("[ERROR]: Couldn't create a font."); 
        }
    } else {
        print_and_exit("[ERROR]: Couldn't add font resource.");
    }
}

struct Parser 
{
    Arena *arena;
    Utf8 input;
    int cursor;
};

struct Token 
{
    char scratchbuffer[1024];
    int len;
};

internal void
eat_character(Parser *parser) 
{
    parser->cursor++;
}

internal int
peek_character(Parser *parser) 
{
    if (parser->cursor < parser->input.len) {
        return parser->input.str[parser->cursor];
    } else {
        return -1;
    }
}

internal void
eat_whitespace(Parser *parser) 
{
    int c = peek_character(parser);
    while (is_whitespace(c)) 
    {
        eat_character(parser);
        c = peek_character(parser);
    }
}

internal Token
parse_line(Parser *parser) 
{
    int c = peek_character(parser);
    Assert(is_alpha(c));

    Token result = {};

    int len = 0;
    while (parser->cursor < parser->input.len && c != '\r' && c != '\n') {
        result.scratchbuffer[len++] = c;
        eat_character(parser);
        c = peek_character(parser);
    }
    result.len = len;

    return result;
}

internal int
parse_integer(Parser *parser) 
{
    int c = peek_character(parser);
    Assert(is_digit(c));

    int result = 0;
    while (parser->cursor < parser->input.len && is_digit(c)) {
        result = result * 10 + atoi(c);
        eat_character(parser);
        c = peek_character(parser);
    }

    return result;
}

struct Parsed_Font_Data 
{
    char id[256];
    char inputfilepath[256];
    char fontname[256];
    char outputfilepath[256];
    int fontsize;
};

Parsed_Font_Data parsed_font_data[1024];
int parsed_font_count;

internal void
parse(Parser *parser)
{
    for (;;)
    {
        eat_whitespace(parser);
        if (parser->cursor >= parser->input.len) {
            break;
        }

        Assert(parsed_font_count < array_count(parsed_font_data));
        Parsed_Font_Data *data = parsed_font_data + parsed_font_count++;

        eat_whitespace(parser);
        Token id = parse_line(parser);
        memory_copy(data->id, id.scratchbuffer, id.len*sizeof(*id.scratchbuffer));

        eat_whitespace(parser);
        Token inputfilename = parse_line(parser);
        char inputfilepath[256];
        str_snprintf(inputfilepath, sizeof(inputfilepath), "%s%.*s", FONT_INPUT_DIRECTORY, inputfilename.len, inputfilename.scratchbuffer);
        static_assert(array_count(inputfilepath) <= array_count(data->inputfilepath));
        memory_copy(data->inputfilepath, inputfilepath, array_count(inputfilepath)*sizeof(*inputfilepath));

        eat_whitespace(parser);
        Token fontname = parse_line(parser);
        memory_copy(data->fontname, fontname.scratchbuffer, fontname.len*sizeof(*fontname.scratchbuffer));

        eat_whitespace(parser);
        Token outputfilename = parse_line(parser);
        char outputfilepath[256];
        str_snprintf(outputfilepath, sizeof(outputfilepath), "%s%.*s", FONT_OUTPUT_DIRECTORY, outputfilename.len, outputfilename.scratchbuffer);
        static_assert(array_count(outputfilepath) <= array_count(data->outputfilepath));
        memory_copy(data->outputfilepath, outputfilepath, array_count(outputfilepath)*sizeof(*outputfilepath));

        eat_whitespace(parser);
        int fontsize = parse_integer(parser);
        data->fontsize = fontsize;
    }
}

int main(void) 
{
    // -------------------------------
    // @Note: init.
    {
        os_init();
        thread_init();
        main_arena = arena_alloc();
    }

    // -------------------------------
    // @Note: parse config.
    Parser *parser = 0;
    {
        Arena *arena = arena_alloc();
        parser = push_struct(arena, Parser);
        parser->arena = arena;

        Utf8 config_file_path = utf8lit("../src/font/config");
        Utf8 input = read_entire_file(parser->arena, config_file_path);
        parser->input = input;
    }
    parse(parser);

    // -------------------------------
    // @Note: create bitmaps.
    for (int i = 0; i < parsed_font_count; ++i) 
    {
        Parsed_Font_Data *data = parsed_font_data + i;

        hdc = CreateCompatibleDC(0);
        if (hdc) 
        {
            BITMAPINFO info = {};
            {
                info.bmiHeader.biSize           = sizeof(info.bmiHeader);
                info.bmiHeader.biWidth          = BITMAP_WIDTH;
                info.bmiHeader.biHeight         = BITMAP_HEIGHT;
                info.bmiHeader.biPlanes         = 1;
                info.bmiHeader.biBitCount       = 32;
                info.bmiHeader.biCompression    = BI_RGB;
                info.bmiHeader.biSizeImage      = 0;
                info.bmiHeader.biXPelsPerMeter  = 0;
                info.bmiHeader.biYPelsPerMeter  = 0;
                info.bmiHeader.biClrUsed        = 0;
                info.bmiHeader.biClrImportant   = 0;
            }

            bitmap = CreateDIBSection(hdc, &info, DIB_RGB_COLORS, &bits, 0, 0);
            SelectObject(hdc, bitmap);

            char output_file_path_tmp[256];
            str_snprintf(output_file_path_tmp, sizeof(output_file_path_tmp), "%s_temp", data->outputfilepath);

            FILE *out = fopen(output_file_path_tmp, "wb");
            if (out) 
            {
                bake_font(data->inputfilepath, data->fontname, out, data->fontsize);
                fclose(out);
                CopyFileA(output_file_path_tmp, data->outputfilepath, FALSE);
                DeleteFileA(output_file_path_tmp);
                printf("%s --> %s  ", data->inputfilepath, data->outputfilepath);
                printf("[OK]\n");
            }
            else 
            {
                fprintf(stderr, "[ERROR]: Couldn't open file.\n");
            }
        } 
        else 
        {
            print_and_exit("[ERROR]: Couldn't create compatible HDC.");
        }
    }

    printf("\nSuccessfully generated font assets.\n");
    return 0;
}
