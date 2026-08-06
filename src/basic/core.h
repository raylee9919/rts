// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RTS_BASIC_CORE_H
#define RTS_BASIC_CORE_H

// Define platform.
//
#ifdef _WIN32
#  define OS_WINDOWS 1
#endif

#ifdef _MSC_VER
#  define COMPILER_CL 1
#endif

// Check validtiy of defined platform.
//
#if OS_WINDOWS
#else
#  error UNDEFINED_OS
#endif

#if COMPILER_CL
#else
#  error UNDEFINED_COMPILER
#endif

// NOTE: Define per-platform stuffs.
//
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
// TODO: GCC
#  define read_only
#endif

#if COMPILER_CL
#  include <intrin.h>
#  define write_barrier() _WriteBarrier()
#elif COMPILER_CLANG
#  include <x86intrin.h>
#  define write_barrier()  // TODO:
#endif

#if COMPILER_CL
#  define per_thread __declspec(thread)
#elif COMPILER_CLANG || COMPILER_GCC
#  define per_thread __thread
#endif


// NOTE: Align Of
//
#if COMPILER_CL || COMPILER_CLANG
#  define align_of(t) __alignof(t)
#elif COMPILER_GCC
#  define align_of(t) __alignof__(t)
#else
#  error ALIGN_OF_NOT_DEFINED_IN_CURRENT_COMPILER
#endif

// NOTE: Address Sanitizer
//
#if COMPILER_MSVC
# if defined(__SANITIZE_ADDRESS__)
#  define ASAN_ENABLED 1
#  define NO_ASAN __declspec(no_sanitize_address)
# else
#  define NO_ASAN
# endif
#elif COMPILER_CLANG
# if defined(__has_feature)
#  if __has_feature(address_sanitizer) || defined(__SANITIZE_ADDRESS__)
#   define ASAN_ENABLED 1
#  endif
# endif
# define NO_ASAN __attribute__((no_sanitize("address")))
#else
# define NO_ASAN
#endif

#if ASAN_ENABLED
extern "C" void __asan_poison_memory_region(void const volatile *addr, size_t size);
extern "C" void __asan_unpoison_memory_region(void const volatile *addr, size_t size);
#  define asan_poison(addr, size)   __asan_poison_memory_region((addr), (size))
#  define asan_unpoison(addr, size) __asan_unpoison_memory_region((addr), (size))
#else
#  define asan_poison(addr, size)   ((void)(addr), (void)(size))
#  define asan_unpoison(addr, size) ((void)(addr), (void)(size))
#endif


// SSE
//
#if defined(_MSC_VER)
#  if defined(_M_AMD64) || ( defined(_M_IX86_FP) && _M_IX86_FP >= 1 )
#    define SSE_ENABLED 1
#  endif
#else
#  if defined(__SSE__)
#    define SSE_ENABLED 1
#  endif
#endif



// 3rd-party include
//
#include <stdint.h>
#include <stdio.h>
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
typedef s8          b8;
typedef s16         b16;
typedef s32         b32;
typedef float       f32; 
typedef double      f64; 

typedef u8 Axis2;
enum
{
    AXIS2_X,
    AXIS2_Y,
    AXIS2_COUNT
};

typedef u8 Axis3;
enum
{
    AXIS3_X,
    AXIS3_Y,
    AXIS3_Z,
    AXIS3_COUNT
};

#define CONCAT(A, B) A##B
#define CONCAT2(A, B) CONCAT(A, B)
#undef assert
#define ASSERT(exp)  if (!(exp)) do { break_debugger(); } while(0)
#define Assert(exp)  if (!(exp)) do { break_debugger(); } while(0)
#define assert(exp)  if (!(exp)) do { break_debugger(); } while(0)
#define assume(exp)  assert(exp)
#define INVALID_CODE_PATH Assert(! "Invalid Code Path")
#define INVALID_DEFAULT_CASE default: { INVALID_CODE_PATH; } break
#define max(a, b) ( ((a) > (b)) ? (a) : (b) )
#define min(a, b) ( ((a) < (b)) ? (a) : (b) )
#define defer_loop(start, end) for(int CONCAT2(_i_,__LINE__) = ((start), 0); CONCAT2(_i_,__LINE__) == 0; (CONCAT2(_i_,__LINE__) += 1, (end)))

// -------------------------------------
// NOTE: Clamp

#define array_count(array) ( sizeof(array) / sizeof(array[0]) )
#define int_from_ptr(p) (u64)(((u8*)p) - 0)
#define ptr_from_int(i) (void*)(((u8*)0) + i)
#define offset_of(type, member) int_from_ptr(&((type *)0)->member)
#define base_from_member(type, member_name, ptr) (type *)((u8 *)(ptr) - offset_of(type, member_name))
#define align_pow2(x,b)      (((x) + (b) - 1)&(~((b) - 1)))
#define align_down_pow2(x,b) ((x)&(~((b) - 1)))
#define clamp(a, lo, hi)    (min(max(a, lo), hi))
#define clamp_lo(a, lo)     (max(a, lo))
#define clamp_hi(a, hi)     (min(a, hi))
#define clamp01(a)          clamp(a, 0, 1)

#define quick_sort(base, type, count, cmp) qsort((base), (count), sizeof(type), (int(*)(const void *, const void *))(cmp))

// ----------------------------------
// NOTE: Memory Operations
#define memory_copy(dst, src, size) memmove((dst), (src), (size))
#define memory_set(dst, byte, size) memset((dst), (byte), (size))
#define memory_compare(a, b, size)  memcmp((a), (b), (size))
#define memory_match(a, b, size)    (memory_compare((a), (b), (size)) == 0)
#define zero_memory(ptr, size)      memory_set((ptr), 0, (size))
#define zero_struct(ptr)            memory_set((ptr), 0, sizeof(*(ptr)))
#define zero_array(ptr, count)      memory_set((ptr), 0, sizeof(*(ptr))*(count))

// ----------------------------------
// NOTE: Data Structure Macros
#define check_null(p) ((p)==0)
#define set_null(p) ((p)=0)
#define check_nil(nil, p) ((p)==0 || (p)==(nil))


// NOTE: List
//
#define sll_push_back_nz(f, l, n, next, zchk, zset) \
    ( ( zchk(f) ) ? \
      ( (f)=(l)=(n), zset((n)->next) ) : \
      ( (l)->next = (n), (l) = (n), zset((n)->next) ) )
#define sll_push_back_n(f, l, n, next)      sll_push_back_nz((f), (l), (n), next, check_null, set_null)
#define sll_push_back(f, l, n)              sll_push_back_nz((f), (l), (n), next, check_null, set_null)

#define sll_push_front_nz(f, l, n, next, zchk, zset) \
    ( ( zchk(f) ) ? \
      ( (f)=(l)=(n), zset((n)->next) ) : \
      ( (n)->next = (f), (f) = (n), zset((n)->next) ) )
#define sll_push_front_n(f, l, n, next)     sll_push_front_nz((f), (l), (n), next, check_null, set_null)
#define sll_push_front(f, l, n)             sll_push_front_n((f), (l), (n), next)

#define sll_pop_front_nz(f, l, next, zset) \
    ( ( (f)==(l) ) ? \
      ( zset(f), zset(l) ) : \
      ( (f)=(f)->next ) )
#define sll_pop_front_n(f, l, next) sll_pop_front_nz(f,l,next,set_null)
#define sll_pop_front(f, l) sll_pop_front_nz(f,l,next,set_null)

#define stack_push_n(f, n, next) ((n)->next=(f), (f)=(n))
#define stack_push(f, n) stack_push_n(f, n, next)
#define stack_pop_nz(f, next, zchk) (zchk(f) ? 0 : ((f)=(f)->next))
#define stack_pop(f) stack_pop_nz(f, next, check_null)

#define list_for_n(f, it, next) \
    for (decltype(f) (it) = (f), _n = (it) ? (it)->next : NULL; \
         (it) != NULL; \
         (it) = _n, _n = (it) ? (it)->next : NULL)

#define list_for(f, it) list_for_n(f, it, next)



#define dll_insert_npz(f,l,p,n,next,prev,zchk,zset) \
    (zchk(f) ? (((f) = (l) = (n)), zset((n)->next), zset((n)->prev)) :\
     zchk(p) ? (zset((n)->prev), (n)->next = (f), (zchk(f) ? (0) : ((f)->prev = (n))), (f) = (n)) :\
     ((zchk((p)->next) ? (0) : (((p)->next->prev) = (n))), (n)->next = (p)->next, (n)->prev = (p), (p)->next = (n),\
      ((p) == (l) ? (l) = (n) : (0))))
#define dll_push_back_npz(f,l,n,next,prev,zchk,zset) dll_insert_npz(f,l,l,n,next,prev,zchk,zset)
#define dll_push_back_np(f,l,n,next,prev) dll_push_back_npz(f,l,n,next,prev,check_null,set_null)
#define dll_remove_npz(f,l,n,next,prev,zchk,zset) (((f)==(n))?\
                                                   ((f)=(f)->next, (zchk(f) ? (zset(l)) : zset((f)->prev))):\
                                                   ((l)==(n))?\
                                                   ((l)=(l)->prev, (zchk(l) ? (zset(f)) : zset((l)->next))):\
                                                   ((zchk((n)->next) ? (0) : ((n)->next->prev=(n)->prev)),\
                                                    (zchk((n)->prev) ? (0) : ((n)->prev->next=(n)->next))))
#define dll_push_back(f,l,n)      dll_push_back_npz(f,l,n,next,prev,check_null,set_null)
#define dll_push_front(f,l,n)     dll_push_back_npz(l,f,n,prev,next,check_null,set_null)
#define dll_insert(f,l,p,n)       dll_insert_npz(f,l,p,n,next,prev,check_null,set_null)
#define dll_remove_np(f,l,n,next,prev) dll_remove_npz(f,l,n,next,prev,check_null,set_null)
#define dll_remove(f,l,n)         dll_remove_npz(f,l,n,next,prev,check_null,set_null)


#define dll_sort_npz(f, l, type, cmp, next, prev, zchk) \
    ( (zchk(f)||zchk(l)) ? (0) : _dll_sort(f, l, sizeof(type), offset_of(type, next), offset_of(type,prev), cmp) )
#define dll_sort(f, l, type, cmp) dll_sort_npz(f, l, type, cmp, next, prev, check_null)

internal void *_dll_np(void *node, u64 np);
internal void _dll_sort(void *first, void *last, u64 size, u64 next, u64 prev, int(*cmp)(void*,void*));


// 
// Defer.
//
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
#define defer(code)  auto CONCAT2(scope_exit_, __LINE__) = scope_exit_make([=](){code;})


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


// NOTE: Constants
//
read_only global const u8 U8_MAX = 0xFF;
read_only global const u8 U8_MIN = 0;

read_only global const u16 U16_MAX = 0xFFFF;
read_only global const u16 U16_MIN = 0;

read_only global const u32 U32_MAX = 0xFFFFFFFF;
read_only global const u32 U32_MIN = 0;

read_only global const u64 U64_MAX = 0xFFFFFFFFFFFFFFFF;
read_only global const u64 U64_MIN = 0;

read_only global const s8 S8_MAX = 0x7F;
read_only global const s8 S8_MIN = -1 - 0x7F;

read_only global const s16 S16_MAX = 0x7FFF;
read_only global const s16 S16_MIN = -1 - 0x7FFF;

read_only global const s32 S32_MAX = 0x7FFFFFFF;
read_only global const s32 S32_MIN = -1 - 0x7FFFFFFF;

read_only global const s64 S64_MAX = 0x7FFFFFFFFFFFFFFF;
read_only global const s64 S64_MIN = -1 - 0x7FFFFFFFFFFFFFFF;

read_only global const f32 F32_MAX = 3.402823e+38f;
read_only global const f32 F32_MIN = -3.402823e+38f;


read_only global const u32 bitmask1  = 0x00000001;
read_only global const u32 bitmask2  = 0x00000003;
read_only global const u32 bitmask3  = 0x00000007;
read_only global const u32 bitmask4  = 0x0000000f;
read_only global const u32 bitmask5  = 0x0000001f;
read_only global const u32 bitmask6  = 0x0000003f;
read_only global const u32 bitmask7  = 0x0000007f;
read_only global const u32 bitmask8  = 0x000000ff;
read_only global const u32 bitmask9  = 0x000001ff;
read_only global const u32 bitmask10 = 0x000003ff;
read_only global const u32 bitmask11 = 0x000007ff;
read_only global const u32 bitmask12 = 0x00000fff;
read_only global const u32 bitmask13 = 0x00001fff;
read_only global const u32 bitmask14 = 0x00003fff;
read_only global const u32 bitmask15 = 0x00007fff;
read_only global const u32 bitmask16 = 0x0000ffff;
read_only global const u32 bitmask17 = 0x0001ffff;
read_only global const u32 bitmask18 = 0x0003ffff;
read_only global const u32 bitmask19 = 0x0007ffff;
read_only global const u32 bitmask20 = 0x000fffff;
read_only global const u32 bitmask21 = 0x001fffff;
read_only global const u32 bitmask22 = 0x003fffff;
read_only global const u32 bitmask23 = 0x007fffff;
read_only global const u32 bitmask24 = 0x00ffffff;
read_only global const u32 bitmask25 = 0x01ffffff;
read_only global const u32 bitmask26 = 0x03ffffff;
read_only global const u32 bitmask27 = 0x07ffffff;
read_only global const u32 bitmask28 = 0x0fffffff;
read_only global const u32 bitmask29 = 0x1fffffff;
read_only global const u32 bitmask30 = 0x3fffffff;
read_only global const u32 bitmask31 = 0x7fffffff;
read_only global const u32 bitmask32 = 0xffffffff;
read_only global const u64 bitmask33 = 0x00000001ffffffffull;
read_only global const u64 bitmask34 = 0x00000003ffffffffull;
read_only global const u64 bitmask35 = 0x00000007ffffffffull;
read_only global const u64 bitmask36 = 0x0000000fffffffffull;
read_only global const u64 bitmask37 = 0x0000001fffffffffull;
read_only global const u64 bitmask38 = 0x0000003fffffffffull;
read_only global const u64 bitmask39 = 0x0000007fffffffffull;
read_only global const u64 bitmask40 = 0x000000ffffffffffull;
read_only global const u64 bitmask41 = 0x000001ffffffffffull;
read_only global const u64 bitmask42 = 0x000003ffffffffffull;
read_only global const u64 bitmask43 = 0x000007ffffffffffull;
read_only global const u64 bitmask44 = 0x00000fffffffffffull;
read_only global const u64 bitmask45 = 0x00001fffffffffffull;
read_only global const u64 bitmask46 = 0x00003fffffffffffull;
read_only global const u64 bitmask47 = 0x00007fffffffffffull;
read_only global const u64 bitmask48 = 0x0000ffffffffffffull;
read_only global const u64 bitmask49 = 0x0001ffffffffffffull;
read_only global const u64 bitmask50 = 0x0003ffffffffffffull;
read_only global const u64 bitmask51 = 0x0007ffffffffffffull;
read_only global const u64 bitmask52 = 0x000fffffffffffffull;
read_only global const u64 bitmask53 = 0x001fffffffffffffull;
read_only global const u64 bitmask54 = 0x003fffffffffffffull;
read_only global const u64 bitmask55 = 0x007fffffffffffffull;
read_only global const u64 bitmask56 = 0x00ffffffffffffffull;
read_only global const u64 bitmask57 = 0x01ffffffffffffffull;
read_only global const u64 bitmask58 = 0x03ffffffffffffffull;
read_only global const u64 bitmask59 = 0x07ffffffffffffffull;
read_only global const u64 bitmask60 = 0x0fffffffffffffffull;
read_only global const u64 bitmask61 = 0x1fffffffffffffffull;
read_only global const u64 bitmask62 = 0x3fffffffffffffffull;
read_only global const u64 bitmask63 = 0x7fffffffffffffffull;
read_only global const u64 bitmask64 = 0xffffffffffffffffull;

enum Texture_Layout
{
    TEXTURE_LAYOUT_INVALID,
    TEXTURE_LAYOUT_RGBA8,
    TEXTURE_LAYOUT_RGB8,
    TEXTURE_LAYOUT_R8,
};

inline u8 align_up(u8 x, u8 alignment) {
    Assert((alignment & (alignment - 1)) == 0);
    return ((x + alignment - 1) & (~(alignment - 1)));
}

inline u16 align_up(u16 x, u16 alignment) {
    Assert((alignment & (alignment - 1)) == 0);
    return ((x + alignment - 1) & (~(alignment - 1)));
}

inline u32 align_up(u32 x, u32 alignment) {
    Assert((alignment & (alignment - 1)) == 0);
    return ((x + alignment - 1) & (~(alignment - 1)));
}

inline u64 align_up(u64 x, u64 alignment) {
    Assert((alignment & (alignment - 1)) == 0);
    return ((x + alignment - 1) & (~(alignment - 1)));
}


// Returns 64 if there's no set bit. That's why TZCNT is better than BSF.
// @Todo: Some old chips might not support tzcnt
inline u64 tzcnt64(u64 x) {
    return _tzcnt_u64(x);
}


#endif // RTS_BASIC_CORE_H
