// Copyright Seong Woo Lee. All Rights Reserved.


#include "basic/string.h"
#include "basic/core.h"
#include "basic/arena.h"
#include "basic/allocator.h"
#include "basic/context.h"

// Faster 'sprintf' than stdlib.
#define STB_SPRINTF_IMPLEMENTATION
#include "basic/vendor/stb_sprintf.h"


//
// c-string
//
u64
string_length(const char *string)
{
    u32 len = 0;
    while (*string++) {
        len++;
    }
    return len;
}

int cstrlen(const char *cstr)
{
    int len = 0;
    while (*cstr++) ++len;
    return len;
}

b32
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

b32
string_equal(char *str1, u64 len1, char *str2) 
{
    return string_equal(str1, len1, str2, string_length(str2));
}

bool
string_equal(const char *str1, char *str2, u64 len2) 
{
    return string_equal((char *)str1, string_length((char *)str1), str2, len2);
}

b32
string_equal(char *str1, char *str2) 
{
    return string_equal(str1, string_length(str1), str2, string_length(str2));
}

String str_copy(String utf, Allocator allocator) {
    String result;
    result.len = utf.len;
    result.str = alloc(sizeof(u8) * (utf.len + 1), allocator);
    memcpy(result.str, utf.str, utf.len);
    result.str[utf.len] = 0;
    return result;
}

//
// Helper Functions.
//
u8 to_upper(u8 c) {
    return (c >= 'a' && c <= 'z') ? (c + 'A' - 'a') : c;
}

u8 to_lower(u8 c) {
    return (c >= 'A' && c <= 'Z') ? (c + 'a' - 'A') : c;
}

b32 is_alpha(u8 c) {
    c &= 0xdf;
    bool result = ((c >= 'A') && (c <= 'Z'));
    return result;
}

b32 is_digit(u8 c) {
    return c >= '0' && c <= '9';
}

b32 is_alnum(u8 c) {
    if (is_alpha(c)) return true;
    if (is_digit(c)) return true;
    return false;
}

b32 is_space(u8 c) {
    return (c == ' ')  || (c == '\n') || (c == '\r') || (c == '\t');
}

void advance(String *s, s64 amount) {
    Assert(amount >= 0);
    Assert(s->len >= amount);
    s->len -= amount;
    s->str += amount;
}

String advance(String s, s64 amount) {
    Assert(amount > 0);
    Assert(s.len >= amount);

    String t;
    t.len = s.len - amount;
    t.str = s.str + amount;
    return t;
}




// # Note: String Constructors
//
String utf8(u8 *str, u64 len)
{
    String result = {};
    result.str = str;
    result.len = len;
    return result;
}

String utf8c(u8 *ptr)
{
    u8 *p = ptr;
    for (;*p; ++p);
    String result = utf8(ptr, p - ptr);
    return result;
}

Utf16
utf16(u16 *str, u64 len)
{
    Utf16 result = {};
    result.str = str;
    result.len = len;
    return result;
}

Utf16
utf16c(u16 *ptr)
{
    u16 *p = ptr;
    for (;*p; ++p);
    Utf16 result = utf16(ptr, p - ptr);
    return result;
}

Utf32
utf32(u32 *str, u64 len)
{
    Utf32 result = {};
    result.str = str;
    result.len = len;
    return result;
}

// # Note: Encoding/Decoding.
//
Unicode_Decode
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

Unicode_Decode
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

u32
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
        str[1] = 0x80 | (codepoint & bitmask6);
        inc = 2;
    }
    else if (codepoint <= 0xFFFF) 
    {
        str[0] = (bitmask3 << 5) | ((codepoint >> 12) & bitmask4);
        str[1] = 0x80 | ((codepoint >> 6) & bitmask6);
        str[2] = 0x80 | ( codepoint       & bitmask6);
        inc = 3;
    }
    else if (codepoint <= 0x10FFFF) 
    {
        str[0] = (bitmask4 << 4) | ((codepoint >> 18) & bitmask3);
        str[1] = 0x80 | ((codepoint >> 12) & bitmask6);
        str[2] = 0x80 | ((codepoint >>  6) & bitmask6);
        str[3] = 0x80 | ( codepoint        & bitmask6);
        inc = 4;
    }
    else 
    {
        str[0] = '?';
        inc = 1;
    }
    return inc;
}

u32
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

String to_utf8(Arena *arena, Utf16 in) {
    String result = {};
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

String to_utf8(Allocator allocator, Utf16 in) {
    String result = {};
    if (in.len)
    {
        u64 cap = in.len*3;
        u8 *str = (u8 *)alloc(sizeof(u8)*(cap + 1), allocator);
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
        result = utf8(str, size);
    }
    return result;
}

Utf16 to_utf16(Arena *arena, String in) {
    Utf16 result = {};
    if (in.len) {
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

Utf16 to_utf16(Allocator allocator, String in) {
    // @Todo: pop...
    Utf16 result = {};
    if (in.len) {
        u64 cap = in.len*2;
        u16 *str = (u16 *)alloc(sizeof(u16) * (cap + 1), allocator);
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
        result = utf16(str, size);
    }
    return result;
}



// # Note: Manipulation.
//
b32
utf8_match(String a, String b, Str_Match_Flags flags)
{
    auto to_forward_slash = [](u8 c) -> u8 {
        if (c == '\\')  return '/';
        else return c;
    };

    b32 result = 0;
    if (a.len == b.len || flags & STR_MATCH_RIGHT_SIDE_SLOPPY)
    {
        result = 1;
        for(s64 i = 0; i < a.len; i += 1)
        {
            b32 match = (a[i] == b[i]);
            if (flags & STR_MATCH_CASE_INSENSITIVE)
            {
                match |= (to_lower(a[i]) == to_lower(b[i]));
            }
            if (flags & STR_MATCH_SLASH_INSENTISIVE)
            {
                match |= (to_forward_slash(a[i]) == to_forward_slash(b[i]));
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

String
utf8_substr(String str, s64 min, s64 max)
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
        s64 swap = min;
        min = max;
        max = swap;
    }
    str.len = max - min;
    str.str += min;
    return str;
}

s64
utf8_find_substr(String haystack, String needle, u64 start_pos, Str_Match_Flags flags)
{
    b32 found = 0;
    s64 found_idx = haystack.len;
    for (s64 i = start_pos; i < haystack.len; i += 1)
    {
        if (i + needle.len <= haystack.len)
        {
            String substr = utf8_substr(haystack, i, i+needle.len);
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

String
utf8_path_chop_last_slash(String string)
{
    Str_Match_Flags flags = STR_MATCH_SLASH_INSENTISIVE | STR_MATCH_FIND_LAST;
    s64 slash_pos = utf8_find_substr(string, S("/"), 0, flags);
    if(slash_pos < string.len)
    {
        string.len = slash_pos;
    }
    return string;
}


b32 equal_nocase(String a, String b) {
    if ( a.len != b.len ) return false;
    for ( s64 i = 0; i < a.len; ++i ) {
        if ( to_lower(a[i]) != to_lower(b[i]) )  return false;
    }
    return true;
}

String copy_string(String s, Allocator allocator) {
    if ( !s.len ) return S("");

    Assert( s.len >= 0 );

    String t;
    t.str = alloc(s.len, allocator);
    t.len = s.len;
    memcpy(t.str, s.str, s.len);
    return t;
}

String eat_spaces( String _s ) {
    String s = _s;
    while ( s.len > 0 && *s.str ) {
        if ( (s[0] != ' ') && (s[0] != 9) ) break;
        advance(&s, 1);
    }

    return s;
}

String eat_trailing_spaces( String _s ) {
    String s = _s;
    while (s.len > 0 && ((s[s.len - 1] == ' ') || (s[s.len - 1] == 9))) {
        s.len -= 1;
    }

    return s;
}

String eat_until_space( String _s ) {
    String s = _s;
    while ( s.len > 0 && *s.str ) {
        if (s[0] == ' ') break;
        if (s[0] == 9  ) break;
        advance(&s, 1);
    }
    return s;
}

b32 begins_with(String s, String prefix) {
    if (s.len < prefix.len)  return false;

    String t = slice(s, 0, prefix.len);

    return t == prefix;
}

b32 ends_with(String s, String suffix) {
    if (s.len < suffix.len)  return false;

    String t = slice(s, s.len - suffix.len, suffix.len);

    return t == suffix;
}

String slice(String s, s64 index, s64 count) {
    Assert(index >= 0);
    Assert(count >= 0);

    if (index >= s.len)  return {};

    if (index + count > s.len) {
        count = s.len - index;
    }

    String c;
    c.str = s.str + index;
    c.len = count;
    return c;
}

static bool is_any(u8 c, String chars) {
    for (s64 i = 0; i < chars.len; ++i) {
        if (c == chars[i])  return true;
    }
    return false;
}

s64 find_index_from_left(String s, u8 byte, s64 start_index) { // @Speed: SIMD
    s64 cursor = start_index;
    
    while ( cursor < s.len ) {
        if ( s[cursor] == byte ) return cursor;
        cursor += 1;
    }

    return -1;
}

s64 find_index_from_right(String s, u8 byte) {
    s64 cursor = s.len - 1;
    while ( cursor >= 0 ) {
        if ( s[cursor] == byte ) return cursor;
        cursor -= 1;
    }

    return -1;
}

s64 find_index_of_any_from_left(String s, String bytes, s64 start_index) {
    s64 cursor = start_index;
    while ( cursor < s.len ) {
        if ( is_any(s[cursor], bytes) ) return cursor;
        cursor += 1;
    }

    return -1;
}

s64 find_index_of_any_from_right(String s, String bytes) {
    s64 cursor = s.len - 1;
    while ( cursor >= 0 ) {
        if ( is_any(s[cursor], bytes) )  return cursor;
        cursor -= 1;
    }

    return -1;
}

Triplet<b32, String, String> split_from_left(String s, u8 byte) {
    s64 index = find_index_from_left(s, byte);
    if ( index == -1 ) return { false, {}, {} };

    String left  = slice(s, 0, index);
    String right = slice(s, index + 1, s.len - index - 1);
    return { true, left, right };
}

Triplet<b32, String, String> split_from_right(String s, u8 byte) {
    s64 index = find_index_from_right(s, byte);
    if ( index == -1 ) return { false, {}, {} };

    String left  = slice(s, 0, index);
    String right = slice(s, index + 1, s.len - index - 1);
    return { true, left, right };
}

String trim_left(String s, String bytes) {
    s64 index = 0;

    for (int i = 0; i < s.len; ++i) {
        if (is_any(s[index], bytes)) index += 1;
        else break;
    }

    return slice(s, index, s.len - index);
}

String trim_right(String s, String bytes) {
    s64 len = s.len;

    for (s64 i = s.len - 1; i >= 0; i--) {
        if (is_any(s[i], bytes)) len -= 1;
        else break;
    }

    return slice(s, 0, len);
}

Triplet<s64, b32, String> int_from_string(String t, s64 base) 
{
    Assert( base == 16 || base <= 10 );

    String s = eat_spaces(t);
    // if ( !*s.str ) return { 0, false, {} };

    s64 sign = 1;

    // Parse sign
    if ( s[0] == '-' )
    {
        sign = -1;
        advance(&s, 1);
        s = eat_spaces(s);
    }
    else if ( s[0] == '+' )
    {
        advance(&s, 1);
        s = eat_spaces(s);
    }

    s64 sum    = 0;
    s64 cursor = 0;

    if ( base == 16 )
    {
        while ( cursor < s.len ) 
        {
            u8 c = s[cursor];

            u8 value;
            if ( is_digit(c) )               value = c - '0';
            else if ( c >= 'a' && c <= 'f' ) value = c - 'a' + 10;
            else if ( c >= 'A' && c <= 'F' ) value = c - 'A' + 10;
            else break;

            sum *= (s64)base;
            sum += sign * (s64)value;

            cursor += 1;
        }
    }
    else 
    {
        while ( cursor < s.len )
        {
            u8 c = s[cursor];
            if ( !is_digit(c) ) break;

            u8 digit = c - '0';
            if ( digit >= base ) break;

            sum *= (s64)base;
            sum += sign * (s64)digit;

            cursor += 1;
        }
    }

    b32 success = ( cursor != 0 );
    advance(&s, cursor);

    return { sum, success, s };
}

//
// Path
//
Pair<String, b32> path_extension(String path) {
    s64 index = find_index_of_any_from_right(path, S(".\\/"));

    if ( index < 0 ) return { {}, false };

    if ( path[index] != '.' ) return { {}, false };

    // Dot right after slash
    u8 previous = path[index - 1];
    if ( index == 0 ) return { {}, false }; // Path can't start with '.' I guess... but let me just be pedantic about it.
    if ( previous == '\\' || previous == '/' ) {
        return { {}, false };
    }

    // Two dots after slash
    if ( previous == '.' ) {
        if ( index == 1 ) return { {}, false };
        u8 two_previous = path[index - 2];
        if ( two_previous == '\\' || two_previous == '/' ) {
            return { {}, false };
        }
    }

    String ext = slice(path, index + 1, path.len - index - 1);

    return { ext, true };
}

String path_strip_filename(String path) {
    s64 index = find_index_of_any_from_right(path, S("\\/"));
    if (index < 0)  return {};
    return slice(path, 0, index + 1);
}

String path_strip_extension(String path) {
    auto [ext, found] = path_extension(path);

    if ( !found ) return path;

    String result = path;
    result.len -= ext.len + 1;
    return result;
}

String tprint(char *fmt, va_list args) {
    String result = {};
    va_list args2;
    va_copy(args2, args);
    u64 needed_bytes = str_vsnprintf(0, 0, fmt, args) + 1;
    result.str = (u8*)alloc(needed_bytes, tctx.temp);
    result.len = needed_bytes - 1;
    str_vsnprintf((char*)result.str, (int)needed_bytes, fmt, args2);
    va_end(args2);
    return result;
}

String tprint(char *fmt, ...) {
    String result = {};
    va_list args;
    va_start(args, fmt);
    result = tprint(fmt, args);
    va_end(args);
    return result;
}

String tprint(String fmt, ...) {
    auto tprintv = [](String fmt, va_list args) {
        u8 *cfmt = alloc(fmt.len + 1, tctx.temp);
        memcpy(cfmt, fmt.str, fmt.len);
        cfmt[fmt.len] = 0;
        return tprint((char*)cfmt, args);
    };

    va_list args;
    va_start(args, fmt);
    String result = tprintv(fmt, args);
    va_end(args);
    return result;
}
