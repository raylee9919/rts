#ifndef RTS_OS_H
#define RTS_OS_H
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Seong Woo Lee $
   $Notice: (C) Copyright %s by Seong Woo Lee. All Rights Reserved. $
   ======================================================================== */


#include "usb_hid_keys.h"


struct Bitmap;
struct Render_Commands;


struct Os_Handle
{
    u64 e[1];
};

typedef u32 Os_File_Access_Flags;
enum
{
    OS_FILE_ACCESS_READ       = (1<<0),
    OS_FILE_ACCESS_WRITE      = (1<<1),
    OS_FILE_ACCESS_SHARED     = (1<<2),
    OS_FILE_ACCESS_CREATE_NEW = (1<<3),
};

typedef u32 Os_File_Flags;
enum
{
    OS_FILE_FLAG_DIRECTORY = (1<<0),
};

struct Os_File_Attributes
{
    Os_File_Flags flags;
    u64 size;
    u64 last_modified;
};

enum Os_System_Path_Kind
{
    OS_SYSTEM_PATH_KIND_NULL,
    OS_SYSTEM_PATH_KIND_INITIAL,
    OS_SYSTEM_PATH_KIND_CURRENT,
    OS_SYSTEM_PATH_KIND_BINARY,
    OS_SYSTEM_PATH_KIND_APPDATA,
    OS_SYSTEM_PATH_KIND_COUNT,
};

struct Os_File_Info
{
    Utf8 name;
    Os_File_Attributes attributes;
};

struct Os_File_Iterator
{
    u8 opaque[1024];
};


// --------------------------
// @Note: OS Include
#if OS_WINDOWS
#  include "os/win32/rts_os_win32.h"
#else
#  error Undefined OS
#endif



// --------------------------
// @Note: Compilers
internal u32
atomic_compare_exchange_u32(u32 volatile *value, u32 _new, u32 expected) 
{
#if COMPILER_CL
    u32 result = _InterlockedCompareExchange((long *)value, _new, expected);
#else
    u32 result = 0;
#endif
    return result;
}

internal u32
atomic_exchange_u32(u32 volatile *value, u32 _new) 
{
#if COMPILER_CL
    u32 result = _InterlockedExchange((long *)value, _new);
#else
    u32 result = 0;
#endif
    return result;
}

internal u64
atomic_exchange_u64(u64 volatile *value, u64 _new) 
{
#if COMPILER_CL
    u64 result = _InterlockedExchange64((long long *)value, _new);
#else
    u64 result = 0;
#endif
    return result;
}

internal u32
atomic_add_u32(u32 volatile *value, u32 addend) 
{
#if COMPILER_CL
    u32 result = _InterlockedExchangeAdd((long *)value, addend);
#else
    u32 result = 0;
#endif
    return result;
}

internal u64
atomic_add_u64(u64 volatile *value, u64 addend) 
{
#if COMPILER_CL
    u64 result = _InterlockedExchangeAdd64((long long *)value, addend);
#else
    u64 result = 0;
#endif
    return result;
}

struct Debug_Executing_Process 
{
    u64 os_handle;
};

struct Debug_Process_State 
{
    b32 started_successfully;
    b32 is_running;
    s32 return_code;
};

struct Os_File_Handle 
{
    void *platform;
};

struct Date_Time
{
    u16 year;
    u8 month;
    u8 day_of_week;
    u8 day;
    u8 hour;
    u8 minute;
    u8 second;
    u8 milliseconds;
};



// -----------------------------------------
// @Note: File
#define OS_FILE_IS_VALID(name) b32 name(Os_Handle file)
typedef OS_FILE_IS_VALID(Os_File_Is_Valid);

#define OS_FILE_OPEN(name) Os_Handle name(Utf8 path, Os_File_Access_Flags flags)
typedef OS_FILE_OPEN(Os_File_Open);

#define OS_FILE_CLOSE(name) void name(Os_Handle file)
typedef OS_FILE_CLOSE(Os_File_Close);

#define OS_FILE_READ(name) u64 name(Os_Handle file, void *dst, u64 size)
typedef OS_FILE_READ(Os_File_Read);

#define OS_FILE_SIZE(name) u64 name(Os_Handle file)
typedef OS_FILE_SIZE(Os_File_Size);

#define OS_FILE_DELETE(name) void name(Utf8 path)
typedef OS_FILE_DELETE(Os_File_Delete);

#define OS_FILE_MOVE(name) void name(Utf8 dst_path, Utf8 src_path)
typedef OS_FILE_MOVE(Os_File_Move);

#define OS_FILE_COPY(name) b32 name(Utf8 dst_path, Utf8 src_path)
typedef OS_FILE_COPY(Os_File_Copy);

#define OS_MAKE_DIRECTORY(name) b32 name(Utf8 path)
typedef OS_MAKE_DIRECTORY(Os_Make_Directory);

// --------------------------------------
// @Note: File Iterator
#define OS_FILE_ITERATOR_BEGIN(name) Os_File_Iterator *name(Arena *arena, Utf8 path)
typedef OS_FILE_ITERATOR_BEGIN(Os_File_Iterator_Begin);

#define OS_FILE_ITERATOR_NEXT(name) b32 name(Arena *arena, Os_File_Iterator *it, Os_File_Info *out_info)
typedef OS_FILE_ITERATOR_NEXT(Os_File_Iterator_Next);

#define OS_FILE_ITERATOR_END(name) void name(Os_File_Iterator *it)
typedef OS_FILE_ITERATOR_END(Os_File_Iterator_End);

// --------------------------------------
// @Note: System Info
#define OS_QUERY_PAGE_SIZE(name) u64 name(void)
typedef OS_QUERY_PAGE_SIZE(Os_Query_Page_Size);

#define OS_STRING_FROM_SYSTEM_PATH_KIND(name) Utf8 name(Arena *arena, Os_System_Path_Kind path)
typedef OS_STRING_FROM_SYSTEM_PATH_KIND(Os_String_From_System_Find_Kind);

#define OS_ATTRIBUTES_FROM_FILE_PATH(name) Os_File_Attributes name(Utf8 path)
typedef OS_ATTRIBUTES_FROM_FILE_PATH(Os_Attributes_From_File_Path);


// ---------------------------------------
// @Note: Memory
#define OS_RESERVE(name) void *name(u64 size)
typedef OS_RESERVE(Os_Reserve);

#define OS_COMMIT(name) b32 name(void *ptr, u64 size)
typedef OS_COMMIT(Os_Commit);

#define OS_DECOMMIT(name) void name(void *ptr, u64 size)
typedef OS_DECOMMIT(Os_Decommit);

#define OS_RELEASE(name) void name(void *ptr, u64 size)
typedef OS_RELEASE(Os_Release);


// -----------------------------------------
// @Note: Abort
#define OS_ABORT(name) void name(void)
typedef OS_ABORT(Os_Abort);

// -----------------------------------------
// @Note: Performance Counter
#define OS_PERF_COUNTER(name) u64 name(void)
typedef OS_PERF_COUNTER(Os_Perf_Counter);

// --------------------------------------
// @Note: Time
#define OS_DATE_TIME_CURRENT(name) Date_Time name(void)
typedef OS_DATE_TIME_CURRENT(Os_Date_Time_Current);



struct OS 
{
    Arena *arena;

    Os_File_Is_Valid    *file_is_valid;
    Os_File_Open        *file_open;
    Os_File_Close       *file_close;
    Os_File_Size        *file_size;
    Os_File_Read        *file_read;
    Os_File_Delete      *file_delete;
    Os_File_Move        *file_move;
    Os_File_Copy        *file_copy;
    Os_Make_Directory   *make_directory;

    Os_File_Iterator_Begin  *file_iterator_begin;
    Os_File_Iterator_Next   *file_iterator_next;
    Os_File_Iterator_End    *file_iterator_end;

    Os_Query_Page_Size              *query_page_size;
    Os_String_From_System_Find_Kind *string_from_system_path_kind;
    Os_Attributes_From_File_Path    *attributes_from_file_path;

    Os_Reserve         *memory_reserve;
    Os_Commit          *memory_commit;
    Os_Decommit        *memory_decommit;
    Os_Release         *memory_release;

    Os_Abort *abort;

    Os_Perf_Counter *perf_counter;
    u64 perf_counter_freq;
    f32 perf_counter_freq_inv;
    f64 perf_counter_freq_inv64;

    Os_Date_Time_Current *date_time_current;

    b32 sleep_is_granular;

    Utf8 binary_path;
    Utf8 initial_path;
    Utf8 appdata_path;
};
global OS os;


// --------------------------------
// @Note: Init
#define OS_INIT(name) void name(void)
typedef OS_INIT(Os_Init);

internal Os_Init os_init;



#endif // RTS_OS_H
