#ifndef RTS_BASE_CORE_H
#define RTS_BASE_CORE_H
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Seong Woo Lee $
   $Notice: (C) Copyright %s by Seong Woo Lee. All Rights Reserved. $
   ======================================================================== */

// --------------------------------------
// @Note: Define platform.
#ifdef _WIN32
#  define OS_WINDOWS 1
#endif

#ifdef _MSC_VER
#  define COMPILER_CL 1
#endif

// --------------------------------------
// @Note: Check platform.
#if OS_WINDOWS
#else
#  error "Undefined OS"
#endif

#if COMPILER_CL
#else
#  error "Undefined Compiler"
#endif

// --------------------------------------
// @Note: Define per-platform stuffs.
#if OS_WINDOWS
#  define break_debugger() __debugbreak()
#else
#  define break_debugger() (*(volatile int *)0 = 0;)
#endif

#if COMPILER_CL || (COMPILER_CLANG && OS_WINDOWS)
#  pragma section(".rdata$", read)
#  define read_only __declspec(allocate(".rdata$"))
#elif (COMPILER_CLANG && OS_LINUX)
#  define read_only __attribute__((section(".rodata")))
#else
// @Todo: GCC
#  define read_only
#endif


// --------------------------------------
// @Note: 3rd-party include
#include <stdint.h>
#include <math.h>
#include <string.h>

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
#define Assert(exp)  if (!(exp)) do { break_debugger(); } while(0)
#define Assume(exp)  Assert(exp)
#define INVALID_CODE_PATH Assert(! "Invalid Code Path")
#define INVALID_DEFAULT_CASE default: { INVALID_CODE_PATH; } break
#define max(a, b) ( (a > b) ? a : b )
#define min(a, b) ( (a < b) ? a : b )
#define array_count(array) ( sizeof(array) / sizeof(array[0]) )
#define offsetof(Type, Member) (size_t)&(((Type *)0)->Member)

// ----------------------------------
// @Note: Memory Operations
#define memory_copy(dst, src, size) memmove((dst), (src), (size))
#define memory_set(dst, byte, size) memset((dst), (byte), (size))
#define memory_compare(a, b, size)  memcmp((a), (b), (size))
#define memory_match(a, b, size)    (memory_compare((a), (b), (size)) == 0)
#define zero_size(ptr, size)        memory_set((ptr), 0, (size))
#define zero_struct(ptr)            memory_set((ptr), 0, sizeof(*(ptr)))
#define zero_array(ptr, count)      memory_set((ptr), 0, sizeof(*(ptr))*(count))


// -----------------------------------------
// @Note: Scope Exit.
template <typename F>
struct Scope_Exit {
    Scope_Exit(F f) : f(f) {}
    ~Scope_Exit() { f(); }
    F f;
};
template <typename F>
Scope_Exit<F> scope_exit_make(F f) {
    return Scope_Exit<F>(f);
};
#define scope_exit(code) \
    auto CONCAT2(scope_exit_, __LINE__) = scope_exit_make([=](){code;})


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


// -------------------------------------------------
// @Note: Constants
read_only global u8 U8_MAX = 0xFF;
read_only global u8 U8_MIN = 0;

read_only global u16 U16_MAX = 0xFFFF;
read_only global u16 U16_MIN = 0;

read_only global u32 U32_MAX = 0xFFFFFFFF;
read_only global u32 U32_MIN = 0;

read_only global u64 U64_MAX = 0xFFFFFFFFFFFFFFFF;
read_only global u64 U64_MIN = 0;

read_only global s8 S8_MAX = 0x7F;
read_only global s8 S8_MIN = -1 - 0x7F;

read_only global s16 S16_MAX = 0x7FFF;
read_only global s16 S16_MIN = -1 - 0x7FFF;

read_only global s32 S32_MAX = 0x7FFFFFFF;
read_only global s32 S32_MIN = -1 - 0x7FFFFFFF;

read_only global s64 S64_MAX = 0x7FFFFFFFFFFFFFFF;
read_only global s64 S64_MIN = -1 - 0x7FFFFFFFFFFFFFFF;

read_only global f32 F32_MAX = 3.402823e+38f;
read_only global f32 F32_MIN = -3.402823e+38f;

#endif // RTS_BASE_CORE_H
