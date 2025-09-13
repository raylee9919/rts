/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Seong Woo Lee $
   $Notice: (C) Copyright %s by Seong Woo Lee. All Rights Reserved. $
   ======================================================================== */


#define STB_SPRINTF_IMPLEMENTATION
#define STB_SPRINTF_DECORATE(name) str_##name
#include "base/vendor/stb_sprintf.h"


internal b32
is_hexdigit(int c) 
{
    return (((c >= '0') && (c <= '9')) || (((c & 0xdf) >= 'A') && ((c & 0xdf) <= 'Z')));
}

internal b32
is_alnum(int c) 
{
    if (is_alpha(c)) return true;
    if (is_digit(c)) return true;
    return false;
}

internal b32
is_whitespace(int c) 
{
    return ( (c == ' ')  || (c == '\t') || (c == '\v') ||
             (c == '\n') || (c == '\f') || (c == '\r') );
}

internal int
atoi(int c) 
{
    return c - '0';
}

internal u64
atoh(int c) 
{
    if (is_digit(c)) 
        return atoi(c);
    else 
        return 10 + ((c & 0xdf) - 'A');
}

internal u64
string_length(const char *string)
{
    u32 len = 0;
    while (*string++) {
        len++;
    }
    return len;
}

internal b32
string_equal(char *str1, u64 len1, char *str2, u64 len2) 
{
    b32 result = (len1 == len2);

    if (result) 
    {
        result = true;
        for (u32 idx = 0; idx < len1; ++idx) {
            if (str1[idx] != str2[idx]) {
                result = false;
                break;
            }
        }
    }

    return result;
}

internal b32
string_equal(char *str1, u64 len1, char *str2) 
{
    return string_equal(str1, len1, str2, string_length(str2));
}

internal b32
string_equal(char *str1, char *str2) 
{
    return string_equal(str1, string_length(str1), str2, string_length(str2));
}

internal s32
s32_from_z_internal(char **at_init)
{
    s32 result = 0;

    char *at = *at_init;
    while ((*at >= '0') && (*at <= '9'))
    {
        result *= 10;
        result += (*at - '0');
        ++at;
    }

    *at_init = at;

    return result;
}

internal s32
s32_from_z(char *at)
{
    char *ignored = at;
    s32 result = s32_from_z_internal(&ignored);
    return result;
}

internal void
copyz(char *src, char *dst) 
{
    u64 len = string_length(src);
    memory_copy(dst, src, len);
    src[len] = 0;
}





// ----------------------------------
// @Note: C-String
internal u64
cstr_length(char *cstr)
{
    u64 result = 0;
    for(;cstr[result]; result++);
    return result;
}

internal Utf8
utf8_copy(Arena *arena, Utf8 utf)
{
    Utf8 result;
    result.len = utf.len;
    result.str = push_array_noz(arena, u8, utf.len + 1);
    memory_copy(result.str, utf.str, utf.len);
    result.str[utf.len] = 0;
    return result;
}

// ----------------------------------
// @Note: Helper Functions.
internal b32
is_alpha(u8 c)
{
    c &= 0xdf;
    b32 result = ((c >= 'A') && (c <= 'Z'));
    return result;
}

internal b32
is_digit(u8 c)
{
    b32 result = (c >= 48 && c <= 57);
    return result;
}

internal b32
is_whitespace(u8 c)
{
    b32 result = (c == ' '  || c == '\t' || c == '\r' ||
                  c == '\n' || c == '\f' || c == '\v');
    return result;
}

internal u8
to_uppercase(u8 c)
{
    return (c >= 'a' && c <= 'z') ? ('A' + (c - 'a')) : c;
}

internal u8
to_lowercase(u8 c)
{
    return (c >= 'A' && c <= 'Z') ? ('a' + (c - 'A')) : c;
}

internal u8
to_forward_slash(u8 c)
{
    return (c == '\\' ? '/' : c);
}

// ----------------------------------
// @Note: String Constructors
internal Utf8
utf8(u8 *str, u64 len)
{
    Utf8 result = {};
    result.str = str;
    result.len = len;
    return result;
}

internal Utf16
utf16(u16 *str, u64 len)
{
    Utf16 result = {};
    result.str = str;
    result.len = len;
    return result;
}

internal Utf32
utf32(u32 *str, u64 len)
{
    Utf32 result = {};
    result.str = str;
    result.len = len;
    return result;
}

// ----------------------------------
// @Note: Encoding/Decoding.
internal Unicode_Decode
utf8_decode(u8 *str, u64 max)
{
    Unicode_Decode result = {1, U32_MAX};
    u8 byte = str[0];
    u8 byte_class = utf8_class[byte >> 3];
    switch (byte_class)
    {
        case 1: {
            result.codepoint = byte;
        } break;
        case 2: {
            if (1 < max)
            {
                u8 cont_byte = str[1];
                if (utf8_class[cont_byte >> 3] == 0)
                {
                    result.codepoint = (byte & bitmask5) << 6;
                    result.codepoint |=  (cont_byte & bitmask6);
                    result.inc = 2;
                }
            }
        } break;
        case 3: {
            if (2 < max)
            {
                u8 cont_byte[2] = {str[1], str[2]};
                if (utf8_class[cont_byte[0] >> 3] == 0 &&
                    utf8_class[cont_byte[1] >> 3] == 0)
                {
                    result.codepoint = (byte & bitmask4) << 12;
                    result.codepoint |= ((cont_byte[0] & bitmask6) << 6);
                    result.codepoint |=  (cont_byte[1] & bitmask6);
                    result.inc = 3;
                }
            }
        } break;
        case 4: {
            if (3 < max)
            {
                u8 cont_byte[3] = {str[1], str[2], str[3]};
                if (utf8_class[cont_byte[0] >> 3] == 0 &&
                    utf8_class[cont_byte[1] >> 3] == 0 &&
                    utf8_class[cont_byte[2] >> 3] == 0)
                {
                    result.codepoint = (byte & bitmask3) << 18;
                    result.codepoint |= ((cont_byte[0] & bitmask6) << 12);
                    result.codepoint |= ((cont_byte[1] & bitmask6) <<  6);
                    result.codepoint |=  (cont_byte[2] & bitmask6);
                    result.inc = 4;
                }
            }
        }
    }
    return result;
}

internal Unicode_Decode
utf16_decode(u16 *str, u64 max)
{
    Unicode_Decode result = {1, U32_MAX};
    result.codepoint = str[0];
    result.inc = 1;
    if (max > 1 && 0xD800 <= str[0] && str[0] < 0xDC00 && 0xDC00 <= str[1] && str[1] < 0xE000)
    {
        result.codepoint = ((str[0] - 0xD800) << 10) | ((str[1] - 0xDC00) + 0x10000);
        result.inc = 2;
    }
    return result;
}

internal u32
utf8_encode(u8 *str, u32 codepoint)
{
    u32 inc = 0;
    if (codepoint <= 0x7F) 
    {
        str[0] = (u8)codepoint;
        inc = 1;
    }
    else if (codepoint <= 0x7FF) 
    {
        str[0] = (bitmask2 << 6) | ((codepoint >> 6) & bitmask5);
        str[1] = bit8 | (codepoint & bitmask6);
        inc = 2;
    }
    else if (codepoint <= 0xFFFF) 
    {
        str[0] = (bitmask3 << 5) | ((codepoint >> 12) & bitmask4);
        str[1] = bit8 | ((codepoint >> 6) & bitmask6);
        str[2] = bit8 | ( codepoint       & bitmask6);
        inc = 3;
    }
    else if (codepoint <= 0x10FFFF) 
    {
        str[0] = (bitmask4 << 4) | ((codepoint >> 18) & bitmask3);
        str[1] = bit8 | ((codepoint >> 12) & bitmask6);
        str[2] = bit8 | ((codepoint >>  6) & bitmask6);
        str[3] = bit8 | ( codepoint        & bitmask6);
        inc = 4;
    }
    else 
    {
        str[0] = '?';
        inc = 1;
    }
    return inc;
}

internal u32
utf16_encode(u16 *str, u32 codepoint)
{
    u32 inc = 1;
    if (codepoint == U32_MAX) 
    {
        str[0] = (u16)'?';
    }
    else if (codepoint < 0x10000) 
    {
        str[0] = (u16)codepoint;
    }
    else 
    {
        u32 v = codepoint - 0x10000;
        str[0] = to_u16_safe(0xD800 + (v >> 10));
        str[1] = to_u16_safe(0xDC00 + (v & bitmask10));
        inc = 2;
    }
    return(inc);
}

// ---------------------------------------
// Note: Conversion.
internal Utf8
to_utf8(Arena *arena, Utf16 in)
{
    Utf8 result = {};
    if (in.len)
    {
        u64 cap = in.len*3;
        u8 *str = push_array_noz(arena, u8, cap + 1);
        u16 *ptr = in.str;
        u16 *opl = ptr + in.len;
        u64 size = 0;
        Unicode_Decode consume;
        for (;ptr < opl; ptr += consume.inc)
        {
            consume = utf16_decode(ptr, opl - ptr);
            size += utf8_encode(str + size, consume.codepoint);
        }
        str[size] = 0;
        arena_pop(arena, (cap - size));
        result = utf8(str, size);
    }
    return result;
}

internal Utf16
to_utf16(Arena *arena, Utf8 in)
{
    Utf16 result = {};
    if (in.len)
    {
        u64 cap = in.len*2;
        u16 *str = push_array_noz(arena, u16, cap + 1);
        u8 *ptr = in.str;
        u8 *opl = ptr + in.len;
        u64 size = 0;
        Unicode_Decode consume = {};
        for (;ptr < opl; ptr += consume.inc)
        {
            consume = utf8_decode(ptr, opl - ptr);
            size += utf16_encode(str + size, consume.codepoint);
        }
        str[size] = 0;
        arena_pop(arena, (cap - size)*2);
        result = utf16(str, size);
    }
    return result;
}



// ------------------------------
// @Note: Manipulation.
internal b32
utf8_match(Utf8 a, Utf8 b, Str_Match_Flags flags)
{
    b32 result = 0;
    if (a.len == b.len || flags & STR_MATCH_RIGHT_SIDE_SLOPPY)
    {
        result = 1;
        for(u64 i = 0; i < a.len; i += 1)
        {
            b32 match = (a.str[i] == b.str[i]);
            if (flags & STR_MATCH_CASE_INSENSITIVE)
            {
                match |= (to_lowercase(a.str[i]) == to_lowercase(b.str[i]));
            }
            if (flags & STR_MATCH_SLASH_INSENTISIVE)
            {
                match |= (to_forward_slash(a.str[i]) == to_forward_slash(b.str[i]));
            }
            if (match == 0)
            {
                result = 0;
                break;
            }
        }
    }
    return result;
}

internal Utf8
utf8_substr(Utf8 str, u64 min, u64 max)
{
    if (max > str.len)
    {
        max = str.len;
    }
    if (min > str.len)
    {
        min = str.len;
    }
    if (min > max)
    {
        u64 swap = min;
        min = max;
        max = swap;
    }
    str.len = max - min;
    str.str += min;
    return str;
}

internal u64
utf8_find_substr(Utf8 haystack, Utf8 needle, u64 start_pos, Str_Match_Flags flags)
{
    b32 found = 0;
    u64 found_idx = haystack.len;
    for (u64 i = start_pos; i < haystack.len; i += 1)
    {
        if (i + needle.len <= haystack.len)
        {
            Utf8 substr = utf8_substr(haystack, i, i+needle.len);
            if (utf8_match(substr, needle, flags))
            {
                found_idx = i;
                found = 1;
                if (! (flags & STR_MATCH_FIND_LAST))
                { break; }
            }
        }
    }
    return found_idx;
}

internal Utf8
utf8_path_chop_last_slash(Utf8 string)
{
    Str_Match_Flags flags = STR_MATCH_SLASH_INSENTISIVE|STR_MATCH_FIND_LAST;
    u64 slash_pos = utf8_find_substr(string, utf8lit("/"), 0, flags);
    if(slash_pos < string.len)
    {
        string.len = slash_pos;
    }
    return string;
}

internal Utf8
utf8fv(Arena *arena, char *fmt, va_list args)
{
    Utf8 result = {};
    va_list args2;
    va_copy(args2, args);
    u64 needed_bytes = str_vsnprintf(0, 0, fmt, args) + 1;
    result.str = push_array_noz(arena, u8, needed_bytes);
    result.len = needed_bytes - 1;
    str_vsnprintf((char*)result.str, (int)needed_bytes, fmt, args2);
    va_end(args2);
    return result;
}

internal Utf8
utf8f(Arena *arena, char *fmt, ...)
{
    Utf8 result = {};
    va_list args;
    va_start(args, fmt);
    result = utf8fv(arena, fmt, args);
    va_end(args);
    return result;
}
