// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RTS_OS_H
#define RTS_OS_H


// Handle
//
struct OS_Handle {
    u64 e[1];
};

// File
//
typedef u32 OS_Access_Flags;
enum
{
    OS_ACCESS_FLAG_READ        =  0x1,
    OS_ACCESS_FLAG_WRITE       =  0x2,
    OS_ACCESS_FLAG_APPEND      =  0x4,
    OS_ACCESS_FLAG_EXECUTE     =  0x8,
    OS_ACCESS_FLAG_SHARE_READ  = 0x10,
    OS_ACCESS_FLAG_SHARE_WRITE = 0x20,
};

struct File_Properties {
    u64  size;
    bool is_directory;
};


// GFX
//
struct OS_Window {
    OS_Window* next;
    OS_Window* prev;
    OS_Handle  handle;
};


// Events
//
enum OS_Key : u32 {
    KEY_NULL,

    KEY_ESC,
    KEY_TILDE,
    KEY_MINUS,
    KEY_EQUAL,
    KEY_BACKSPACE,
    KEY_TAB,
    KEY_SPACE,
    KEY_RETURN,
    KEY_CTRL,
    KEY_SHIFT,
    KEY_ALT,
    KEY_UP,
    KEY_LEFT,
    KEY_DOWN,
    KEY_RIGHT,
    KEY_DELETE,
    KEY_PAGE_UP,
    KEY_PAGE_DOWN,
    KEY_HOME,
    KEY_END,
    KEY_SLASH,
    KEY_BACK_SLASH,
    KEY_PERIOD,
    KEY_COMMA,
    KEY_QUOTE,
    KEY_LEFT_BRACKET,
    KEY_RIGHT_BRACKET,
    KEY_INSERT,
    KEY_SEMICOLON,
    KEY_PAUSE,
    KEY_CAPS_LOCK,
    KEY_NUMS_LOCK,
    KEY_SCROLL_LOCK,
    KEY_MENU,

    // Numpad
    KEY_NUM_DIVIDE,
    KEY_NUM_MULTIPLY,
    KEY_NUM_SUBTRACT,
    KEY_NUM_ADD,
    KEY_NUM_DECIMAL,

    // Equivalent to '0'~'9'.
    KEY_0 = 48,
    KEY_1 = 49,
    KEY_2 = 50,
    KEY_3 = 51,
    KEY_4 = 52,
    KEY_5 = 53,
    KEY_6 = 54,
    KEY_7 = 55,
    KEY_8 = 56,
    KEY_9 = 57,

    // Equivalent to 'A'~'Z'.
    KEY_A = 65,
    KEY_B = 66,
    KEY_C = 67,
    KEY_D = 68,
    KEY_E = 69,
    KEY_F = 70,
    KEY_G = 71,
    KEY_H = 72,
    KEY_I = 73,
    KEY_J = 74,
    KEY_K = 75,
    KEY_L = 76,
    KEY_M = 77,
    KEY_N = 78,
    KEY_O = 79,
    KEY_P = 80,
    KEY_Q = 81,
    KEY_R = 82,
    KEY_S = 83,
    KEY_T = 84,
    KEY_U = 85,
    KEY_V = 86,
    KEY_W = 87,
    KEY_X = 88,
    KEY_Y = 89,
    KEY_Z = 90,

    KEY_F1,
    KEY_F2,
    KEY_F3,
    KEY_F4,
    KEY_F5,
    KEY_F6,
    KEY_F7,
    KEY_F8,
    KEY_F9,
    KEY_F10,
    KEY_F11,
    KEY_F12,
    KEY_F13,
    KEY_F14,
    KEY_F15,
    KEY_F16,
    KEY_F17,
    KEY_F18,
    KEY_F19,
    KEY_F20,
    KEY_F21,
    KEY_F22,
    KEY_F23,
    KEY_F24,

    // Mouse
    KEY_MOUSE_LEFT,
    KEY_MOUSE_RIGHT,
    KEY_MOUSE_MIDDLE,
};

typedef u32 OS_Modifiers;
enum
{
    OS_MODIFIER_CTRL  = 0x1,
    OS_MODIFIER_SHIFT = 0x2,
    OS_MODIFIER_ALT   = 0x4,
};

enum OS_Event_Kind : u32 {
    OS_EVENT_NULL,

    OS_EVENT_PRESS,
    OS_EVENT_RELEASE,
    OS_EVENT_MOUSE_MOVE,
    OS_EVENT_TEXT,
    OS_EVENT_SCROLL,
    OS_EVENT_WINDOW_CLOSE,
    OS_EVENT_FILE_DROP,
};

struct OS_Event {
    OS_Event*       next;
    OS_Event*       prev;

    OS_Event_Kind   kind;
    OS_Modifiers    modifiers;

    OS_Handle       window;
    OS_Key          key;
    v2              position;
    v2              delta;
    u32             codepoint;
    bool            is_repeat;
    u16             repeat_count;
};

// @Temporary
//
#define OS_WORK_CALLBACK(name) void name(void *param)
typedef OS_WORK_CALLBACK(Work_Callback);

struct Work {
    Work_Callback* callback;
    void* param;
};

struct Work_Queue {
    Work works[4192];

    u32 volatile index_to_write;
    u32 volatile index_to_read;

    u32 volatile completion_count;
    u32 volatile completion_goal;

    OS_Handle semaphore;
};

// Global OS State
//
struct OS_State {
    Arena* arena;

    // Events
    Arena*      event_arena;
    OS_Event*   first_event;
    OS_Event*   last_event;
    OS_Event*   first_free_event;
    OS_Event*   last_free_event;

    // Input
    OS_Key      vk_to_key[512];
    b8          key_is_down[512];
    b8          key_was_down[512];
    
    // Counter
    f64 qpc_rcp_freq64;
    f32 qpc_rcp_freq32;

    // Path
    String binary_path;
    String initial_path;
    String appdata_path;

    // @Temporary
    Work_Queue work_queue;
};
global OS_State* os;


// OS Include
//
#if OS_WINDOWS
#  include "os/win32/os_win32.h"
#else
#  error Undefined OS
#endif


// Init
internal void               os_init();

// Memory
internal void*              os_reserve(u64 size);
internal bool               os_commit(void* ptr, u64 size);
internal void               os_decommit(void* ptr, u64 size);
internal void               os_release(void* ptr, u64 size);
internal void*              os_heap_alloc(u64 size);
internal void               os_heap_free(void* ptr);

// System Info.
internal u32                os_query_core_count();
internal u32                os_query_page_size();
internal u32                os_query_caret_blink_time();

// Counter
internal u64                os_counter();
internal u64                os_counter_freq();
internal f32                os_counter_freq_rcp();
internal f64                os_counter_freq_rcp64();

// Handle Translation
internal bool               operator == (OS_Handle& l, OS_Handle& r);
internal OS_Handle          os_handle_from_hwnd(HWND hwnd);
internal OS_Handle          os_handle_from_win32_handle(HANDLE handle);
internal HWND               hwnd_from_os_handle(OS_Handle handle);
internal HANDLE             win32_handle_from_os_handle(OS_Handle handle);

// File
internal OS_Handle          os_open_file(String path, OS_Access_Flags flags);
internal void               os_close_file(OS_Handle file);
internal u64                os_read_file(OS_Handle file, u64 offset, u64 size, void* out);
internal bool               os_delete_file(String path);
internal bool               os_copy_file(String dst, String src);
internal File_Properties    os_get_file_properties(OS_Handle file);
internal File_Properties    os_get_file_properties(String path);
internal u64                os_get_file_size(OS_Handle file);
internal u64                os_get_file_size(String path);
internal bool               os_create_directory(String path);
internal bool               os_directory_exists(String path);

// GFX
internal void               gfx_init();
internal OS_Handle          os_create_window(int w, int h, String name);
internal v2                 os_get_window_size(OS_Handle window);
internal v2                 os_get_mouse_position(OS_Handle window);

// Events
internal void               os_poll_events();
internal OS_Event*          os_push_event();
internal void               os_remove_event(OS_Event* event);
internal void               os_clear_events();

// Main Entry
#if !defined(BUILD_NO_ENTRY) || !BUILD_NO_ENTRY
int main_entry(int argc, char** argv);
#endif


#endif // RTS_OS_H
