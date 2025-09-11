/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Sung Woo Lee $
   $Notice: (C) Copyright 2025 by Sung Woo Lee. All Rights Reserved. $
   ======================================================================== */



    
#define CONCAT_(A, B) A##B
#define CONCAT(A, B) CONCAT_(A, B)

#define Assert(expression)  if(!(expression)) { *(volatile int *)0 = 0; }
#define INVALID_CODE_PATH Assert(!"Invalid Code Path")
#define INVALID_DEFAULT_CASE default: { INVALID_CODE_PATH } break

#define max(a, b) ( (a > b) ? a : b )
#define min(a, b) ( (a < b) ? a : b )
#define arraycount(array) ( sizeof(array) / sizeof(array[0]) )

#define offsetof(Type, Member) (size_t)&(((Type *)0)->Member)

#define zerostruct(PTR, STRUCT) zerosize((PTR), sizeof(STRUCT))
#define zeroarray(ARR, COUNT) zerosize((ARR), (COUNT) * sizeof(ARR[0]))
inline void zerosize(void *ptr, mmm size) {
    u8 *byte = (u8 *)ptr;
    while (size--) *byte++ = 0;
}

#define copy_array(src, dst, count) copy((src), (dst), sizeof(*src)*(count))
inline void copy(void *src_, void *dst_, umm size) {
    u8 *src = (u8 *)src_;
    u8 *dst = (u8 *)dst_;
    while (size--) {*dst++ = *src++;}
}

template <typename F>
struct ScopeExit {
    ScopeExit(F f) : f(f) {}
    ~ScopeExit() { f(); }
    F f;
};
template <typename F>
ScopeExit<F> MakeScopeExit(F f) {
    return ScopeExit<F>(f);
};

#define STRING_JOIN2(arg1, arg2) DO_STRING_JOIN2(arg1, arg2)
#define DO_STRING_JOIN2(arg1, arg2) arg1 ## arg2
#define SCOPE_EXIT(code) \
    auto STRING_JOIN2(scope_exit_, __LINE__) = MakeScopeExit([=](){code;})

inline bool is_alpha(int c) {
    c &= 0xdf;
    return ((c >= 'A') && (c <= 'Z'));
}

inline bool is_digit(int c) {
    return ((c >= '0') && (c <= '9'));
}

inline bool is_hexdigit(int c) {
    return (((c >= '0') && (c <= '9')) || (((c & 0xdf) >= 'A') && ((c & 0xdf) <= 'Z')));
}

inline bool is_alnum(int c) {
    if (is_alpha(c)) return true;
    if (is_digit(c)) return true;
    return false;
}

inline bool is_whitespace(int c) {
    return ( (c == ' ')  || (c == '\t') || (c == '\v') ||
             (c == '\n') || (c == '\f') || (c == '\r') );
}

inline int atoi(int c) {
    return c - '0';
}

inline umm atoh(int c) {
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

inline void copyz(char *src, char *dst) {
    umm len = string_length(src);
    copy_array(src, dst, len);
    src[len] = 0;
}

internal const char *
get_filename_from_filepath(const char *filepath)
{
    const char *result = 0;
    for (const char *at = filepath; *at != 0; ++at)
        if (*at == '\\' || *at == '/')
            result = at;
    result = result ? result + 1 : 0;
    return result;
}

umm f32_to_raw(f32 value) {
    umm foo = *(umm *)&value;
    umm result = foo & 0xffffffff;
    return result;
}
