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

#if COMPILER_CL
#  include <intrin.h>
#  define write_barrier() _WriteBarrier()
#elif COMPILER_CLANG
#  include <x86intrin.h>
#  define write_barrier()  // @Todo:
#endif

#if COMPILER_CL
#  define per_thread __declspec(thread)
#elif COMPILER_CLANG || COMPILER_GCC
#  define per_thread __thread
#endif

#define read_timer_stamp_counter() __rdtsc()

// -----------------------------------------------
// @Note: Align Of
#if COMPILER_CL || COMPILER_CLANG
#  define align_of(t) __alignof(t)
#elif COMPILER_GCC
#  define align_of(t) __alignof__(t)
#else
#  error align_of is not defined in this compiler.
#endif




// --------------------------------------
// @Note: 3rd-party include
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
#define no_name_mangle  extern "C"

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
typedef size_t      mmm;
typedef uintptr_t   umm;
typedef intptr_t    smm;


#define CONCAT(A, B) A##B
#define CONCAT2(A, B) CONCAT(A, B)
#define Assert(exp)  if (!(exp)) do { break_debugger(); } while(0)
#define INVALID_CODE_PATH Assert(! "Invalid Code Path")
#define INVALID_DEFAULT_CASE default: { INVALID_CODE_PATH; } break
#define max(a, b) ( ((a) > (b)) ? (a) : (b) )
#define min(a, b) ( ((a) < (b)) ? (a) : (b) )

// -------------------------------------
// @Note: Clamp

#define array_count(array) ( sizeof(array) / sizeof(array[0]) )
#define offsetof(Type, Member) (size_t)&(((Type *)0)->Member)
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
// @Note: Memory Operations
#define memory_copy(dst, src, size) memmove((dst), (src), (size))
#define memory_set(dst, byte, size) memset((dst), (byte), (size))
#define memory_compare(a, b, size)  memcmp((a), (b), (size))
#define memory_match(a, b, size)    (memory_compare((a), (b), (size)) == 0)
#define zero_memory(ptr, size)      memory_set((ptr), 0, (size))
#define zero_struct(ptr)            memory_set((ptr), 0, sizeof(*(ptr)))
#define zero_array(ptr, count)      memory_set((ptr), 0, sizeof(*(ptr))*(count))

// ----------------------------------
// @Note: Data Structure Macros
#define check_null(p) ((p)==0)
#define set_null(p) ((p)=0)
#define check_nil(nil, p) ((p)==0 || (p)==(nil))


#define sll_push_back_nz(f, l, n, next, zchk, zset) \
    ( ( zchk(f) ) ? \
      ( (f)=(l)=(n), zset((n)->next) ) : \
      ( (l)->next = (n), (l) = (n), zset((n)->next) ) )
#define sll_push_back(f, l, n) sll_push_back_nz((f), (l), (n), next, check_null, set_null)

#define sll_pop_front_nz(f, l, next, zset) \
    ( ( (f)==(l) ) ? \
      ( zset(f), zset(l) ) : \
      ( (f)=(f)->next ) )
#define sll_pop_front(f, l) sll_pop_front_nz(f,l,next,set_null)

#define list_for_n(f, it, next) for (decltype(f) (it) = (f); (it) != 0; (it) = (it)->next)
#define list_for(f, it)  list_for_n(f, it, next)

// ---------------------------------------
// @Note: Doubly Linked List
//            You don't use a sentinel, and you pass 'item_first' pointer as a parameter to a function.
//        In the function, you remove the first item. What happens next? When you pop out of the function, 
//        you lost track of the 'actual' first item in the list. Maybe you can resolve this by double pointer macro though.
//            I'm using sentinel for now. Sentinel is always there, which is good. You won't lost the state of the list.
//        The phase(?) when you push or remove is always the same, and it makes implementation easeier, but I dont' consider 
//        this a critical gain. It just makes your life easier for a short time. The problem I face with sentinel is that, 
//        you need to allocate and initialize to use it. Before you start using it, you have to check if the sentinel is null (allocation), 
//        and you have to check if sentinel's next, prev pointer is pointing itself (initialization). Allocating goes through arena, which 
//        will init to zero. Thus, if next and prev pointers are zero, it means it isn't initted. Then the API will initialze it. What about
//        push_noz? Ah...
//            I may want composible data structure, even though I don't know what it is at this point. To achieve such thing, 
//        initialization might be a constraint. I don't know. Must do some study and experiments.
//
#define dll_init_npz(s, next, prev, ifz) \
    ( ifz(s) ? \
      (0) : \
      ((s)->next = (s), (s)->prev = (s)) )
#define dll_init(s) dll_init_npz(s, next, prev, check_null)

#define dll_push_back_npz(s, n, next, prev, ifz) \
    ( ifz(s) ? \
      (0) : \
      ( (ifz((s)->next) || ifz((s)->prev)) ? \
        ( dll_init_npz(s, next, prev, ifz), ((n)->prev=(s)->prev, (n)->next=(s), (s)->prev->next=(n), (s)->prev=(n)) ) : \
        ( (n)->prev=(s)->prev, (n)->next=(s), (s)->prev->next=(n), (s)->prev=(n) ) ) )
#define dll_push_back_np(s, n, next, prev) dll_push_back_npz(s, n, next, prev, check_null)
#define dll_push_back(s, n) dll_push_back_npz(s, n, next, prev, check_null)

#define dll_remove_npz(s, n, next, prev, zchk, zset) \
    ( ((n) == (s)) ? \
      (0) : \
      ( zchk(n) ? \
        (0) : \
        ( ((zchk((n)->prev) ? (0) : (n)->prev->next = (n)->next)), \
          ((zchk((n)->next) ? (0) : (n)->next->prev = (n)->prev)) ) ) )
#define dll_remove(s, n) dll_remove_npz(s, n, next, prev, check_null, set_null)


#define dll_sort_npz(s, type, cmp, next, prev, zchk) \
    ( zchk(s) ? (0) : _dll_sort(s, sizeof(type), offset_of(type, next), offset_of(type,prev), cmp) )
#define dll_sort(s, type, cmp) dll_sort_npz(s, type, cmp, next, prev, check_null)

internal void *_dll_np(void *node, u64 np);
internal void _dll_sort(void *sentinel, u64 size, u64 next, u64 prev, int(*cmp)(void*,void*));



// ----------------------------------
// @Todo: Dynamic Array. Is it a good idea...?
#define DYNAMIC_ARRAY_DATA(TYPE)\
    struct {\
        Arena *arena;\
        TYPE  *base;\
        u64   count_cur;\
        u64   count_max;\
    }

#define Dynamic_Array(TYPE)\
    union {\
        DYNAMIC_ARRAY_DATA(TYPE);\
        TYPE *payload;\
    }

#define DAR_RESERVE_INIT 64

#define dar_init(A, ARENA)\
    (A)->arena = ARENA;\
    (A)->base = (decltype((A)->payload))push_size(ARENA, sizeof(decltype(*(A)->payload)) * DAR_RESERVE_INIT);\
    (A)->count_cur = 0;\
    (A)->count_max = DAR_RESERVE_INIT

// @Todo: Fragmentation.
#define dar_push(A, ITEM)\
    if (((A)->count_cur >= (A)->count_max)) {\
        void *new_base = push_size((A)->arena, sizeof(decltype(*(A)->payload)) * ((A)->count_max << 1));\
        memory_copy( new_base, (A)->base, sizeof(decltype(*(A)->payload)) * (A)->count_max );\
        (A)->count_max <<= 1;\
    }\
    *(decltype((A)->payload))( ((decltype((A)->payload))((A)->base)) + ((A)->count_cur++) ) = ITEM;

// Sets the length of an array to at least N.
#define dar_reserve(A, N)\
    if ((A)->count_max < N) {\
        void *new_base = push_size( (A)->arena, sizeof(decltype(*(A)->payload)) * N );\
        memory_copy( new_base, (A)->base, sizeof(decltype(*(A)->payload)) * (A)->count_cur );\
        (A)->count_max = N; \
    }



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

// -------------------------------------------------
// @Note: Helper Functions
internal u16 to_u16_safe(u32 x);
internal u32 to_u32_safe(u64 x);
internal s32 to_s32_safe(s64 x);
internal umm to_raw(f32 val);




// -------------------------------------------------
// @Note: Constants
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

read_only global const u32 bit1  = (1<<0);
read_only global const u32 bit2  = (1<<1);
read_only global const u32 bit3  = (1<<2);
read_only global const u32 bit4  = (1<<3);
read_only global const u32 bit5  = (1<<4);
read_only global const u32 bit6  = (1<<5);
read_only global const u32 bit7  = (1<<6);
read_only global const u32 bit8  = (1<<7);
read_only global const u32 bit9  = (1<<8);
read_only global const u32 bit10 = (1<<9);
read_only global const u32 bit11 = (1<<10);
read_only global const u32 bit12 = (1<<11);
read_only global const u32 bit13 = (1<<12);
read_only global const u32 bit14 = (1<<13);
read_only global const u32 bit15 = (1<<14);
read_only global const u32 bit16 = (1<<15);
read_only global const u32 bit17 = (1<<16);
read_only global const u32 bit18 = (1<<17);
read_only global const u32 bit19 = (1<<18);
read_only global const u32 bit20 = (1<<19);
read_only global const u32 bit21 = (1<<20);
read_only global const u32 bit22 = (1<<21);
read_only global const u32 bit23 = (1<<22);
read_only global const u32 bit24 = (1<<23);
read_only global const u32 bit25 = (1<<24);
read_only global const u32 bit26 = (1<<25);
read_only global const u32 bit27 = (1<<26);
read_only global const u32 bit28 = (1<<27);
read_only global const u32 bit29 = (1<<28);
read_only global const u32 bit30 = (1<<29);
read_only global const u32 bit31 = (1<<30);
read_only global const u32 bit32 = (u32)(1<<31);
read_only global const u64 bit33 = (1ull<<32);
read_only global const u64 bit34 = (1ull<<33);
read_only global const u64 bit35 = (1ull<<34);
read_only global const u64 bit36 = (1ull<<35);
read_only global const u64 bit37 = (1ull<<36);
read_only global const u64 bit38 = (1ull<<37);
read_only global const u64 bit39 = (1ull<<38);
read_only global const u64 bit40 = (1ull<<39);
read_only global const u64 bit41 = (1ull<<40);
read_only global const u64 bit42 = (1ull<<41);
read_only global const u64 bit43 = (1ull<<42);
read_only global const u64 bit44 = (1ull<<43);
read_only global const u64 bit45 = (1ull<<44);
read_only global const u64 bit46 = (1ull<<45);
read_only global const u64 bit47 = (1ull<<46);
read_only global const u64 bit48 = (1ull<<47);
read_only global const u64 bit49 = (1ull<<48);
read_only global const u64 bit50 = (1ull<<49);
read_only global const u64 bit51 = (1ull<<50);
read_only global const u64 bit52 = (1ull<<51);
read_only global const u64 bit53 = (1ull<<52);
read_only global const u64 bit54 = (1ull<<53);
read_only global const u64 bit55 = (1ull<<54);
read_only global const u64 bit56 = (1ull<<55);
read_only global const u64 bit57 = (1ull<<56);
read_only global const u64 bit58 = (1ull<<57);
read_only global const u64 bit59 = (1ull<<58);
read_only global const u64 bit60 = (1ull<<59);
read_only global const u64 bit61 = (1ull<<60);
read_only global const u64 bit62 = (1ull<<61);
read_only global const u64 bit63 = (1ull<<62);
read_only global const u64 bit64 = (1ull<<63);

#endif // RTS_BASE_CORE_H
