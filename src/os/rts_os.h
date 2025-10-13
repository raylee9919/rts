#ifndef RTS_OS_H
#define RTS_OS_H
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Seong Woo Lee $
   $Notice: (C) Copyright %s by Seong Woo Lee. All Rights Reserved. $
   ======================================================================== */


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

struct Date_Time
{
    u16 year;
    u8  month;
    u8  day_of_week;
    u8  day;
    u8  hour;
    u8  minute;
    u8  second;
    u8  milliseconds;
};

// # Note: Event
//
typedef u16 Os_Event_Type;
enum 
{
    OS_EVENT_NULL = 0,

    OS_EVENT_PRESS,
    OS_EVENT_RELEASE,

    OS_EVENT_TEXT,

    OS_EVENT_MOUSE_MOVE,
    OS_EVENT_MOUSE_SCROLL,
};

typedef u16 Os_Key;
enum
{
    OS_KEY_NULL         = 0,

    OS_KEY_MOUSE_LEFT   = 1,
    OS_KEY_MOUSE_RIGHT  = 2,
    OS_KEY_MOUSE_MIDDLE = 4,

    OS_KEY_ESC,
    OS_KEY_GRAVE_ACCENT,
    OS_KEY_MINUS,
    OS_KEY_EQUAL,
    OS_KEY_BACKSPACE,
    OS_KEY_TAB,
    OS_KEY_SPACE,
    OS_KEY_ENTER,
    OS_KEY_CTRL,
    OS_KEY_SHIFT,
    OS_KEY_ALT,
    OS_KEY_UP,
    OS_KEY_LEFT,
    OS_KEY_DOWN,
    OS_KEY_RIGHT,
    OS_KEY_DELETE,
    OS_KEY_PAGE_UP,
    OS_KEY_PAGE_DOWN,
    OS_KEY_HOME,
    OS_KEY_END,
    OS_KEY_FORWARD_SLASH,
    OS_KEY_PERIOD,
    OS_KEY_COMMA,
    OS_KEY_QUOTE,
    OS_KEY_LEFT_BRACKET,
    OS_KEY_RIGHT_BRACKET,
    OS_KEY_INSERT,
    OS_KEY_SEMICOLON,

    OS_KEY_0 = 48,
    OS_KEY_1 = 49,
    OS_KEY_2 = 50,
    OS_KEY_3 = 51,
    OS_KEY_4 = 52,
    OS_KEY_5 = 53,
    OS_KEY_6 = 54,
    OS_KEY_7 = 55,
    OS_KEY_8 = 56,
    OS_KEY_9 = 57,

    OS_KEY_A = 65,
    OS_KEY_B = 66,
    OS_KEY_C = 67,
    OS_KEY_D = 68,
    OS_KEY_E = 69,
    OS_KEY_F = 70,
    OS_KEY_G = 71,
    OS_KEY_H = 72,
    OS_KEY_I = 73,
    OS_KEY_J = 74,
    OS_KEY_K = 75,
    OS_KEY_L = 76,
    OS_KEY_M = 77,
    OS_KEY_N = 78,
    OS_KEY_O = 79,
    OS_KEY_P = 80,
    OS_KEY_Q = 81,
    OS_KEY_R = 82,
    OS_KEY_S = 83,
    OS_KEY_T = 84,
    OS_KEY_U = 85,
    OS_KEY_V = 86,
    OS_KEY_W = 87,
    OS_KEY_X = 88,
    OS_KEY_Y = 89,
    OS_KEY_Z = 90,

    OS_KEY_F1  = 112,
    OS_KEY_F2  = 113,
    OS_KEY_F3  = 114,
    OS_KEY_F4  = 115,
    OS_KEY_F5  = 116,
    OS_KEY_F6  = 117,
    OS_KEY_F7  = 118,
    OS_KEY_F8  = 119,
    OS_KEY_F9  = 120,
    OS_KEY_F10 = 121,
    OS_KEY_F11 = 122,
    OS_KEY_F12 = 123,
    OS_KEY_F13 = 124,
    OS_KEY_F14 = 125,
    OS_KEY_F15 = 126,
    OS_KEY_F16 = 127,
    OS_KEY_F17 = 128,
    OS_KEY_F18 = 129,
    OS_KEY_F19 = 130,
    OS_KEY_F20 = 131,
    OS_KEY_F21 = 132,
    OS_KEY_F22 = 133,
    OS_KEY_F23 = 134,
    OS_KEY_F24 = 135,
};

typedef u16 Os_Modifiers;
enum
{
    OS_MODIFIER_CTRL  = (1<<0),
    OS_MODIFIER_SHIFT = (1<<1),
    OS_MODIFIER_ALT   = (1<<2)
};

struct Os_Event
{
    Os_Event        *next;
    Os_Event        *prev;

    Os_Event_Type   type;
    Os_Key          key;
    Os_Modifiers    modifiers;
    v2              position;
    v2              delta;
    u32             character;
};




// # Note: OS Include
//
#if OS_WINDOWS
#  include "os/win32/rts_os_win32.h"
#else
#  error Undefined OS
#endif

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

#define OS_CARET_BLINK_TIME(name) u32 name(void)
typedef OS_CARET_BLINK_TIME(Os_Caret_Blink_Time);

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

// --------------------------------------
// @Note: Event Poll
#define OS_EVENT_POLL(name) void name(void)
typedef OS_EVENT_POLL(Os_Event_Poll);

#define OS_GET_MODIFIERS(name) Os_Modifiers name(void)
typedef OS_GET_MODIFIERS(Os_Get_Modifiers);





struct OS 
{
    Arena                           *arena;

    Os_File_Is_Valid                *file_is_valid;
    Os_File_Open                    *file_open;
    Os_File_Close                   *file_close;
    Os_File_Size                    *file_size;
    Os_File_Read                    *file_read;
    Os_File_Delete                  *file_delete;
    Os_File_Move                    *file_move;
    Os_File_Copy                    *file_copy;
    Os_Make_Directory               *make_directory;

    Os_File_Iterator_Begin          *file_iterator_begin;
    Os_File_Iterator_Next           *file_iterator_next;
    Os_File_Iterator_End            *file_iterator_end;

    Os_Query_Page_Size              *query_page_size;
    Os_Caret_Blink_Time             *caret_blink_time;
    Os_String_From_System_Find_Kind *string_from_system_path_kind;
    Os_Attributes_From_File_Path    *attributes_from_file_path;

    Os_Reserve                      *memory_reserve;
    Os_Commit                       *memory_commit;
    Os_Decommit                     *memory_decommit;
    Os_Release                      *memory_release;

    Os_Abort                        *abort;

    Os_Event_Poll                   *event_poll;
    Os_Get_Modifiers                *get_modifiers;

    Os_Perf_Counter                 *perf_counter;
    u64                             perf_counter_freq;
    f32                             perf_counter_freq_inv;
    f64                             perf_counter_freq_inv64;

    Os_Date_Time_Current            *date_time_current;

    b32                             sleep_is_granular;

    Utf8                            binary_path;
    Utf8                            initial_path;
    Utf8                            appdata_path;

    Arena                           *event_arena;
    Os_Event                        *event_free_first;
    Os_Event                        *event_free_last;
    Os_Event                        *event_first;
    Os_Event                        *event_last;

    Os_Key                          key_table[256];
    b32                             key_is_down[256];
    b32                             key_toggled[256];
    v2                              mouse_position_last;
};
global OS *os;


// # Note: Init
//
#define OS_INIT(name) void name(void)
typedef OS_INIT(Os_Init);
internal Os_Init os_init;

// # Note: Event
//
internal Os_Event *
os_event_alloc(void)
{
    Os_Event *event = os->event_free_first;

    if (event == NULL)
    {
        event = push_struct(os->event_arena, Os_Event);
    }
    else
    {
        zero_memory(event, sizeof(*event));
        sll_pop_front(os->event_free_first, os->event_free_last);
    }

    return event;
}

internal void
os_event_release(Os_Event *event)
{
    sll_push_back(os->event_free_first, os->event_free_last, event);
}

internal void
os_event_consume(Os_Event *event)
{
    dll_remove(os->event_first, os->event_last, event);
    os_event_release(event);
}

internal void
os_event_list_clear(void)
{
    for (Os_Event *event = os->event_first, *next; event != NULL; event = next)
    {
        next = event->next;
        os_event_consume(event);
    }
}



#endif // RTS_OS_H
