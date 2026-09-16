// Copyright Seong Woo Lee. All Rights Reserved.


#ifndef RTS_STRING_H
#define RTS_STRING_H

#include <stdarg.h>

#include "basic/core.h"
#include "basic/arena.h"
#include "basic/allocator.h"


// Faster 'sprintf' than stdlib.
#define STB_SPRINTF_DECORATE(name) str_##name
#include "basic/vendor/stb_sprintf.h"


#if OS_WINDOWS
#  define PATH_SEPARATOR '\\'
#else
#  define PATH_SEPARATOR '/'
#endif



//
// UTF-8 string.
//
struct String {
    u8 *str;
    s64 len;

    bool operator == (const String& other) {
        if (len != other.len)  return false;
        return memcmp(str, other.str, len) == 0;
    }
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


u64 string_length(const char *string);
int cstrlen(const char *cstr);
b32 string_equal(char *str1, u64 len1, char *str2, u64 len2);
b32 string_equal(char *str1, u64 len1, char *str2);
bool string_equal(const char *str1, char *str2, u64 len2);
b32 string_equal(char *str1, char *str2);

bool is_alpha(int c);
bool is_digit(int c);
bool is_hexdigit(int c);
bool is_alnum(int c);
bool is_whitespace(int c);
int  atoi(int c);
int  atoh(int c);
u8   to_uppercase(u8 c);
u8   to_lowercase(u8 c);
u8   to_forward_slash(u8 c);

//
// Constructors
//
#define S(str) String{(u8 *)str, sizeof(str) - 1}
String utf8(u8 *str, u64 len);
String utf8c(u8 *ptr);
String str_copy(String str, Allocator allocator);
Utf16 utf16(u16 *str, u64 len);
Utf16 utf16c(u16 *ptr);
Utf32 utf32(u32 *str, u64 len);

//
// Encoding/Decoding
//
Unicode_Decode utf8_decode(u8 *str, u64 max);
Unicode_Decode utf16_decode(u16 *str, u64 max);
u32 utf8_encode(u8 *str, u32 codepoint);
u32 utf16_encode(u16 *str, u32 codepoint);

//
// Conversion
//
String to_utf8(Arena *arena, Utf16 in);
String to_utf8(Allocator allocator, Utf16 in);
Utf16 to_utf16(Arena *arena, String in);
Utf16 to_utf16(Allocator allocator, String in);

//
// Manipulation (Old)
//
b32 utf8_match(String a, String b, Str_Match_Flags flags);
String utf8_substr(String str, s64 min, s64 max);
s64 utf8_find_substr(String haystack, String needle, u64 start_pos, Str_Match_Flags flags);
String utf8_path_chop_last_slash(String string);


/* Manipulation */
String  eat_trailing_spaces(String s);
b32     begins_with(String s, String prefix);
b32     ends_with(String s, String suffix);
String  slice(String s, s64 index, s64 count);
s64     find_index_of_any_from_right(String s, String bytes);


/* String Format */
String tprint(char *fmt, va_list args);
String tprint(char *fmt, ...);
String tprint(String fmt, ...);


/* Path */
String path_strip_filename(String path);


#endif // RTS_STRING_H
