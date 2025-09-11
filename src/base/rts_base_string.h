#ifndef RTS_STRING_H
#define RTS_STRING_H
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

internal b32 is_alpha(int c);
internal b32 is_digit(int c);
internal b32 is_hexdigit(int c);
internal b32 is_alnum(int c) ;
internal b32 is_whitespace(int c);
internal int atoi(int c);
internal umm atoh(int c);
internal umm string_length(const char *string);
internal b32 string_equal(char *str1, umm len1, char *str2, umm len2);
internal b32 string_equal(char *str1, umm len1, char *str2);
internal b32 string_equal(char *str1, char *str2);
internal String wrapz(char *z);
internal s32 s32_from_z_internal(char **at_init);
internal s32 s32_from_z(char *at);
internal void copyz(char *src, char *dst);

#endif // RTS_STRING_H
