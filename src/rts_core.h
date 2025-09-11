#ifndef RTS_BASE_CORE_H
#define RTS_BASE_CORE_H
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Seong Woo Lee $
   $Notice: (C) Copyright %s by Seong Woo Lee. All Rights Reserved. $
   ======================================================================== */

#include <stdint.h>
#include <stddef.h>
#include <limits.h>
#include <float.h>

#define KB(value) (   value  * 1024ll)
#define MB(value) (KB(value) * 1024ll)
#define GB(value) (MB(value) * 1024ll)
#define TB(value) (GB(value) * 1024ll)

#define internal        static
#define global          static
#define local_persist   static

typedef int8_t      s8;  
typedef int16_t     s16; 
typedef int32_t     s32; 
typedef int64_t     s64; 
typedef uint8_t     u8;  
typedef uint16_t    u16; 
typedef uint32_t    u32; 
typedef uint64_t    u64; 
typedef s32         b32;
typedef float       f32; 
typedef double      f64; 
typedef size_t      mmm;
typedef uintptr_t   umm;
typedef intptr_t    smm;
struct Buffer 
{
    umm count;
    u8 *data;
};
typedef Buffer String;


#define CONCAT(A, B) A##B
#define CONCAT2(A, B) CONCAT(A, B)
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

// -----------------------------------------
// @Note: Scope Exit.
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
#define SCOPE_EXIT(code) \
    auto CONCAT2(scope_exit_, __LINE__) = MakeScopeExit([=](){code;})


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


#define F32_MIN FLT_MIN
#define F32_MAX FLT_MAX

#endif // RTS_BASE_CORE_H
