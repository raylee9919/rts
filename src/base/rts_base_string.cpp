/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Seong Woo Lee $
   $Notice: (C) Copyright %s by Seong Woo Lee. All Rights Reserved. $
   ======================================================================== */

internal b32
is_alpha(int c) 
{
    c &= 0xdf;
    return ((c >= 'A') && (c <= 'Z'));
}

internal b32 
is_digit(int c) 
{
    return ((c >= '0') && (c <= '9'));
}

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

internal umm
atoh(int c) 
{
    if (is_digit(c)) 
        return atoi(c);
    else 
        return 10 + ((c & 0xdf) - 'A');
}

internal umm
string_length(const char *string)
{
    u32 count = 0;
    while (*string++) {
        count++;
    }
    return count;
}

internal b32
string_equal(char *str1, umm len1, char *str2, umm len2) 
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
string_equal(char *str1, umm len1, char *str2) 
{
    return string_equal(str1, len1, str2, string_length(str2));
}

internal b32
string_equal(char *str1, char *str2) 
{
    return string_equal(str1, string_length(str1), str2, string_length(str2));
}

internal String
wrapz(char *z)
{
    String result;

    result.count = string_length(z);
    result.data = (u8 *)z;

    return result;
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
    umm len = string_length(src);
    memory_copy(dst, src, len);
    src[len] = 0;
}
