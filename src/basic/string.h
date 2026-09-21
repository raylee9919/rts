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

    bool operator != (const String& other) {
        if (len != other.len)  return true;
        return memcmp(str, other.str, len) != 0;
    }

    u8 operator [] (s64 i) { Assert(i < len); return str[i]; }
    explicit operator bool() const { return str && (*str) && len > 0; }
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


s64 cstrlen(const char *cstr);
b32 string_equal(char *str1, u64 len1, char *str2, u64 len2);
b32 string_equal(char *str1, u64 len1, char *str2);
bool string_equal(const char *str1, char *str2, u64 len2);
b32 string_equal(char *str1, char *str2);

//
// Constructors
//
#define S(str) String{(u8 *)str, sizeof(str) - 1}
String utf8(u8 *str, u64 len);
String utf8c(u8 *ptr);
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
u8      to_upper(u8 c);
u8      to_lower(u8 c);
b32     is_alpha(u8 c);
b32     is_digit(u8 c);
b32     is_alnum(u8 c);
b32     is_space(u8 c);
void    advance(String *s, s64 amount = 1);
String  advance(String  s, s64 amount = 1);

b32     equal_nocase(String a, String b);

String  copy_string(String s, Allocator allocator);

String  eat_spaces( String s );
String  eat_trailing_spaces( String s );
String  eat_until_space( String s );

b32     begins_with(String s, String prefix);
b32     ends_with(String s, String suffix);
String  slice(String s, s64 index, s64 count);

s64     find_index_from_left(String s, u8 byte, s64 start_index = 0);
s64     find_index_from_right(String s, u8 byte);
s64     find_index_of_any_from_left(String s, String bytes, s64 start_index = 0);
s64     find_index_of_any_from_right(String s, String bytes);

Triplet<b32, String, String>  split_from_left(String s, u8 byte);
Triplet<b32, String, String>  split_from_right(String s, u8 byte);

String trim_left(String s, String bytes = S(" \t\n\r"));
String trim_right(String s, String bytes = S(" \t\n\r"));

Triplet<s64, b32, String> int_from_string(String s, s64 base = 10);


/* String Format */
String tprint(char *fmt, va_list args);
String tprint(char *fmt, ...);
String tprint(String fmt, ...);


/*    Path    */

Pair<String, b32> path_extension(String path);

// Returns a slice of the input string with file name removd, but including path separator.
String path_strip_filename(String path);

String path_strip_extension(String path);


#endif // RTS_STRING_H
