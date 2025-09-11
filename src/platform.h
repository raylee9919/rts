/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Sung Woo Lee $
   $Notice: (C) Copyright %s by Sung Woo Lee. All Rights Reserved. $
   ======================================================================== */

#include "usb_hid_keys.h"

struct Bitmap;
struct Render_Commands;

///////////////////////////////////////////////////////////////////////////////
//
// Compilers
//
internal u32
atomic_compare_exchange_u32(u32 volatile *value, u32 _new, u32 expected) 
{
#if __MSVC
    u32 result = _InterlockedCompareExchange((long *)value, _new, expected);
#else
    u32 result = 0;
#endif
    return result;
}

internal u32
atomic_exchange_u32(u32 volatile *value, u32 _new) 
{
#if __MSVC
    u32 result = _InterlockedExchange((long *)value, _new);
#else
    u32 result = 0;
#endif
    return result;
}

internal u64
atomic_exchange_u64(u64 volatile *value, u64 _new) 
{
#if __MSVC
    u64 result = _InterlockedExchange64((long long *)value, _new);
#else
    u64 result = 0;
#endif
    return result;
}

internal u32
atomic_add_u32(u32 volatile *value, u32 addend) 
{
#if __MSVC
    u32 result = _InterlockedExchangeAdd((long *)value, addend);
#else
    u32 result = 0;
#endif
    return result;
}

internal u64
atomic_add_u64(u64 volatile *value, u64 addend) 
{
#if __MSVC
    u64 result = _InterlockedExchangeAdd64((long long *)value, addend);
#else
    u64 result = 0;
#endif
    return result;
}

struct Debug_Executing_Process {
    u64 os_handle;
};

struct Debug_Process_State {
    b32 started_successfully;
    b32 is_running;
    s32 return_code;
};

struct Platform_File_Handle {
    void *platform;
};

struct Platform_Time {
    u16 year;
    u16 month;
    u16 dayofweek;
    u16 day;
    u16 hour;
    u16 minute;
    u16 second;
    u16 ms;
};

enum Platform_File_Type {
    TYPE_INVALID,
    TYPE_FILE,
    TYPE_DIRECTORY,
};
struct Platform_File_Info {
    Platform_File_Type type;
    char filename[256];
};
struct Platform_File_List {
    Platform_File_Info infos[256];
    umm count;
};

enum Platform_Open_File_Flag {
    Open_File_Read  = 0x1,
    Open_File_Write = 0x2,
};

#define PLATFORM_LIST_FILES(NAME) Platform_File_List NAME(char *path)
typedef PLATFORM_LIST_FILES(Platform_List_Files);

#define PLATFORM_OPEN_FILE(NAME) Platform_File_Handle NAME(char *filepath, u32 flags)
typedef PLATFORM_OPEN_FILE(Platform_Open_File);

#define PLATFORM_CLOSE_FILE(NAME) void NAME(Platform_File_Handle *handle)
typedef PLATFORM_CLOSE_FILE(Platform_Close_File);

#define PLATFORM_GET_FILE_SIZE(NAME) u32 NAME(Platform_File_Handle *handle)
typedef PLATFORM_GET_FILE_SIZE(Platform_Get_File_Size);

#define PLATFORM_READ_FROM_FILE(NAME) void NAME(Platform_File_Handle *handle, void *dst, umm size)
typedef PLATFORM_READ_FROM_FILE(Platform_Read_From_File);

#define PLATFORM_READ_ENTIRE_FILE(NAME) Buffer NAME(char *filepath)
typedef PLATFORM_READ_ENTIRE_FILE(Platform_Read_Entire_File);

#define PLATFORM_COPY_FILE(NAME) void NAME(char *src, char *dst)
typedef PLATFORM_COPY_FILE(Platform_Copy_File);

#define PLATFORM_FREE_MEMORY(NAME) void NAME(void *memory)
typedef PLATFORM_FREE_MEMORY(Platform_Free_Memory);

#define PLATFORM_READ_CPU_TIMER(NAME) u64 NAME()
typedef PLATFORM_READ_CPU_TIMER(Platform_Read_Cpu_Timer);

#define PLATFORM_GET_SYSTEM_TIME(NAME) Platform_Time NAME()
typedef PLATFORM_GET_SYSTEM_TIME(Platform_Get_System_Time);

#define PLATFORM_GET_LAST_WRITE_TIME(NAME) u64 NAME(char *filepath)
typedef PLATFORM_GET_LAST_WRITE_TIME(Platform_Get_Last_Write_Time);

struct Platform_Work_Queue;
#define PLATFORM_WORK_QUEUE_CALLBACK(Name) void Name(Platform_Work_Queue *queue, void *data)
typedef PLATFORM_WORK_QUEUE_CALLBACK(Platform_Work_QueueCallback);

typedef void Platform_Add_Entry(Platform_Work_Queue *queue, Platform_Work_QueueCallback *callback, void *data);
typedef void Platform_Complete_All_Work(Platform_Work_Queue *queue);

struct Platform_Api 
{
    Platform_Add_Entry          *add_entry;
    Platform_Complete_All_Work  *complete_all_work;
    
    Platform_List_Files         *list_files;
    Platform_Open_File          *open_file;
    Platform_Close_File         *close_file;
    Platform_Get_File_Size      *get_file_size;
    Platform_Read_From_File     *read_from_file;
    Platform_Read_Entire_File   *read_entire_file;
    Platform_Copy_File          *copy_file;
    Platform_Free_Memory        *free_memory;
    Platform_Get_System_Time    *get_system_time;
    Platform_Read_Cpu_Timer     *read_cpu_timer;
    Platform_Get_Last_Write_Time *get_last_write_time;

    u64 cpu_timer_frequency;
};

struct Game_Memory 
{
    // @Important: Spec memory to be initialized to zero.
    u8 *total_memory;
    umm total_memory_size;

    Platform_Work_Queue *high_priority_queue;
    Platform_Work_Queue *low_priority_queue;

    b32 executable_reloaded;
    Platform_Api os;
};

#define GAME_UPDATE_AND_RENDER(name) void name(Game_Memory *game_memory,      \
                                               Input *input,                  \
                                               Event_Queue *event_queue,      \
                                               Render_Commands *render_commands)
typedef GAME_UPDATE_AND_RENDER(Game_Update_And_Render);
