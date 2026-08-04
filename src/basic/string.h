// Copyright Seong Woo Lee. All Rights Reserved.


#ifndef RTS_STRING_H
#define RTS_STRING_H


// Faster 'sprintf' than stdlib.
#define STB_SPRINTF_DECORATE(name) str_##name
#include "basic/vendor/stb_sprintf.h"



//
// UTF-8 string.
//
struct String {
    u8 *str;
    s64 len;
};


struct Utf16 {
    u16 *str;
    u64 len;
};

struct Utf32 {
    u32 *str;
    u64 len;
};

struct Unicode_Decode {
    u32 inc;
    u32 codepoint;
};

read_only global u8 utf8_class[32] = 
{
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,2,2,2,2,3,3,4,5,
};

typedef u32 Str_Match_Flags;
enum
{
    STR_MATCH_CASE_INSENSITIVE  = (1<<0),
    STR_MATCH_RIGHT_SIDE_SLOPPY = (1<<1),
    STR_MATCH_SLASH_INSENTISIVE = (1<<2),
    STR_MATCH_FIND_LAST         = (1<<3),
    STR_MATCH_KEEP_EMPTIES      = (1<<4),
};



//
// Constructors.
//
#define utf8lit(str) utf8((u8 *)str, sizeof(str) - 1)
#define S(str) String{(u8 *)str, sizeof(str) - 1}
internal String utf8(u8 *str, u64 len);
internal String utf8c(u8 *ptr);
internal Utf16 utf16(u16 *str, u64 len);
internal Utf16 utf16c(u16 *ptr);
internal Utf32 utf32(u32 *str, u64 len);

//
// Encoding/Decoding.
//
internal Unicode_Decode utf8_decode(u8 *str, u64 max);
internal Unicode_Decode utf16_decode(u16 *str, u64 max);
internal u32 utf8_encode(u8 *str, u32 codepoint);
internal u32 utf16_encode(u16 *str, u32 codepoint);

//
// Conversion.
//
internal String to_utf8(Arena *arena, Utf16 in);
internal Utf16 to_utf16(Arena *arena, String in);
internal Utf16 to_utf16(Allocator allocator, String in);

//
// Manipulation.
//
internal b32 utf8_match(String a, String b, Str_Match_Flags flags);
internal String utf8_substr(String str, s64 min, s64 max);
internal s64 utf8_find_substr(String haystack, String needle, u64 start_pos, Str_Match_Flags flags);
internal String utf8_path_chop_last_slash(String string);

//
// Chop/Slash Helpers.
//
internal String utf8_skip_whitespace(String str);
internal String utf8_chop_whitespace(String str);
internal String utf8_skip_chop_whitespace(String str);

//
// Operators
//
bool operator == (String l, String r);


#endif // RTS_STRING_H
