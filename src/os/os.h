// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RTS_OS_H
#define RTS_OS_H


struct Thread;
struct Thread_Group;


// Main Entry
#if !defined(BUILD_NO_ENTRY) || !BUILD_NO_ENTRY
int main_entry(int argc, char** argv);
#endif


// Handle
//
struct OS_Handle {
    u64 e[1];
};


// Thread
//
struct Thread {
    OS_Handle handle;
    u64       tid;

    void (*proc)(void *);
    void *param;
};

struct Work_Entry {
    Work_Entry *next;

    void       (*proc)(void*);
    void       *param;

    s32        thread_index; // thread of an index in the group that handled the work.
};

struct Work_List {
    Work_Entry *first;
    Work_Entry *last;
    s32         count;

    Mutex       mutex;
    Semaphore   semaphore;
};

struct Worker_Info {
    Work_List     available;

    Thread        thread;

    Thread_Group *group;
    s32           index; // Synonym of 'lane' index.
};

//
// This is a single producer thread group. It simple cycles 'next_worker_index'  
// and adds work to corresponding worker info's work list. Each thread has its 
// own queue so that cache coherency isn't a problem. Job-stealing is a @Todo.
//
struct Thread_Group {
    String          name;

    Arena          *arena;
    Temporary_Arena temp;

    s32             next_worker_index;

    Worker_Info    *worker_info;
    s32             count;
    
    // @Todo: This is bad. Cache coherency. Multiple threads will write to those and invalidate the cache.
    s64 volatile    completed;
    s64 volatile    added;

    b32             initted;
    b32             should_shutdown;
};

enum Wait_Result : u8 {
    WAIT_RESULT_SUCCESS = 0,
    WAIT_RESULT_ERROR   = 1,
    WAIT_RESULT_TIMEOUT = 2,
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
    OS_Window *next;
    OS_Window *prev;

    OS_Handle  handle;
};


// Events
//
#define KEY_GOOD_CAP 256
enum OS_Key : u16 {
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


// Thing (Discriminated Union)
//
enum OS_Thing_Kind : u8 {
    OS_THING_KIND_INVALID = 0,

    OS_THING_KIND_THREAD,

    OS_THING_KIND_OPL // one-past-last
};

struct OS_Thing {
    OS_Thing_Kind kind;
    OS_Thing *next;
    OS_Thing *prev;
    union {
        Thread thread;
    };
};


// GUID
//
struct Guid {
    union {
        struct {
            u32 data1;
            u16 data2;
            u16 data3;
            u8  data4[8];
        };
        u8   u[16];
        u16 _16[8];
        u32 _32[4];
        u64 _64[2];
    };

    bool operator == (const Guid& other) {
        return memcmp(u, other.u, sizeof(u)) == 0;
    }

    bool operator != (const Guid& other) {
        return memcmp(u, other.u, sizeof(u)) == 1;
    }
};
global read_only const Guid NULL_GUID = {0};


// Global OS State
//
struct OS_State {
    Arena *arena;

    // Platform-specific
    void *native;

    // Events
    Arena*      event_arena;
    OS_Event*   first_event;
    OS_Event*   last_event;
    OS_Event*   first_free_event;
    OS_Event*   last_free_event;

    // Input
    OS_Key      vk_to_key[512];
#if 0
    b8          key_is_down[512];
    b8          key_was_down[512];
#endif
    
    // Path
    String binary_path;
    String initial_path;
    String appdata_path;

    // Thing Free List
    OS_Thing *first_free_thing;
    OS_Thing *last_free_thing;

    // Thing list
    OS_Thing *first_thing[OS_THING_KIND_OPL - 1];
    OS_Thing *last_thing[OS_THING_KIND_OPL - 1];
};
global OS_State *os;


// APIs
//
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

// Time
internal f64                time_s();
internal f64                time_ms();
internal f64                time_us();

// Handle Translation
internal bool               operator == (OS_Handle& l, OS_Handle& r);
internal OS_Handle          os_handle_from_hwnd(HWND hwnd);
internal OS_Handle          os_handle_from_win32_handle(HANDLE handle);
internal HWND               hwnd_from_os_handle(OS_Handle handle);
internal HANDLE             win32_handle_from_os_handle(OS_Handle handle);
internal void*              get_native_window_handle(OS_Handle window);

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
internal void               os_gfx_init();
internal OS_Handle          os_window_create(int w, int h, String name);
internal void               os_window_toggle_fullscreen(OS_Handle window);
internal v2                 os_window_size(OS_Handle window);
internal v2                 os_get_mouse_position(OS_Handle window);

// Event
internal void               os_poll_events();
internal OS_Event*          os_push_event();
internal void               os_remove_event(OS_Event* event);
internal void               os_clear_events();

// Mutex (non-re-entrant, meaning, 'lock -> lock' is invalid)
internal void               mutex_create(Mutex *mutex);
internal void               mutex_destroy(Mutex *mutex);
internal void               mutex_lock(Mutex *mutex);
internal void               mutex_unlock(Mutex *mutex);

// Condition Variable
internal void               condvar_create(Condvar *condvar);
internal void               condvar_destroy(Condvar *condvar);
internal Wait_Result        condvar_sleep(Condvar *condvar, Mutex *mutex, s64 timeout_ms);
internal void               condvar_wake_one(Condvar *condvar);
internal void               condvar_wake_all(Condvar *condvar);

// Semaphore
internal void               semaphore_create(Semaphore *semaphore);
internal void               semaphore_destroy(Semaphore *semaphore);
internal void               semaphore_signal(Semaphore *semaphore);
internal Wait_Result        semaphore_wait(Semaphore *semaphore, s32 milliseconds); // Pass in negative number to wait indefinitely.

// Thread
internal Thread             thread_launch(void (*proc)(void *), void *param);
internal bool               thread_join(Thread thread, s32 endt_us);
internal void               thread_set_name(String name);

// Thread Group
internal void               thread_group_init(Thread_Group *group, s32 num_threads, Arena *arena, String group_name);
internal void               thread_group_shutdown(Thread_Group *group);
internal void               thread_group_add_work(Thread_Group *group, void (*proc)(void *), void *param);
internal void               thread_group_complete_all_work(Thread_Group *group);

// UUID/GUID
internal Guid               guid_generate();

// Atomic
internal void               atomic_increment(volatile s32 *x);

template<typename F> 
internal void parallel_for(Thread_Group *group, s64 count, F&& func);


#endif // RTS_OS_H
