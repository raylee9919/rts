// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef OPERATING_SYSTEM_H
#define OPERATING_SYSTEM_H

#include "basic/core.h"
#include "basic/arena.h"
#include "basic/allocator.h"
#include "basic/string.h"
#include "basic/array.h"
#include "math/math.h"

#if OS_WINDOWS
#  include "os/win32/win32.h"
#else
#  error Undefined OS
#endif


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


// GFX
//
struct OS_Window {
    OS_Window *next;
    OS_Window *prev;

    OS_Handle  handle;
};


// Events
//
// @Cleanup
#if 0
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
#endif

enum Event_Type : u32 {
    EVENT_UNINITIALIZED       = 0,
    EVENT_KEYBOARD            = 1,
    EVENT_TEXT_INPUT          = 2,
    EVENT_WINDOW              = 3,
    EVENT_MOUSE_WHEEL         = 4,
    EVENT_QUIT                = 5,
    EVENT_DRAG_AND_DROP_FILES = 6,
};

typedef u32 Key_State;
enum {
    KEY_STATE_NONE  = 0x0,
    KEY_STATE_DOWN  = 0x1,
    KEY_STATE_START = 0x4,
    KEY_STATE_END   = 0x8,
};

// We reserve 32 buttons for each gamepad.
#define GAMEPAD_BUTTON_COUNT 32

enum Key_Code : u32 {
    KEY_UNKNOWN     = 0,

    // Non-tectual keys that have placements in the ASCII table
    // (and thus in Unicode):
    
    KEY_BACKSPACE   = 8,
    KEY_TAB         = 9,
    KEY_LINEFEED    = 10,
    KEY_ENTER       = 13,
    KEY_ESCAPE      = 27,
    KEY_SPACEBAR    = 32,

    // The letters A-Z live in here as well and may be returned
    // by keyboard events.

    KEY_DELETE      = 127,

    KEY_ARROW_UP    = 128,
    KEY_ARROW_DOWN  = 129,
    KEY_ARROW_LEFT  = 130,
    KEY_ARROW_RIGHT = 131,

    KEY_PAGE_UP     = 132,
    KEY_PAGE_DOWN   = 133,

    KEY_HOME        = 134,
    KEY_END         = 135,

    KEY_INSERT      = 136,

    KEY_PAUSE       = 137,
    KEY_SCROLL_LOCK = 138,

    KEY_ALT,
    KEY_CTRL,
    KEY_SHIFT,
    KEY_CMD,

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

    KEY_PRINT_SCREEN,

    MOUSE_BUTTON_LEFT,
    MOUSE_BUTTON_MIDDLE,
    MOUSE_BUTTON_RIGHT,

    MOUSE_WHEEL_UP,
    MOUSE_WHEEL_DOWN,

    MOUSE_BUTTON_EXTRA_1,
    MOUSE_BUTTON_EXTRA_2,

    // We reserve button codes for up to 4 gamepads.
    GAMEPAD_0_BEGIN,
    GAMEPAD_0_END = GAMEPAD_0_BEGIN + GAMEPAD_BUTTON_COUNT,
    GAMEPAD_1_BEGIN,
    GAMEPAD_1_END = GAMEPAD_1_BEGIN + GAMEPAD_BUTTON_COUNT,
    GAMEPAD_2_BEGIN,
    GAMEPAD_2_END = GAMEPAD_2_BEGIN + GAMEPAD_BUTTON_COUNT,
    GAMEPAD_3_BEGIN,
    GAMEPAD_3_END = GAMEPAD_3_BEGIN + GAMEPAD_BUTTON_COUNT,

    // WARNING(swL)
    //
    // We make an array whose size is controlled 
    // by the last enum value in this array, so if you make 
    // really big values to match Unicode code points, our 
    // memmory usage will become quite sorry.

    KEY_CODE_MAX
};

struct Event {
    struct Modifier_Flags {
        union {
            u32 packed = 0;
            struct {
                b8 shift_pressed;
                b8 ctrl_pressed;
                b8 alt_pressed;
                b8 cmd_meta_pressed;
            };
        };
    };

    Event_Type type = EVENT_UNINITIALIZED;

    u32 key_pressed; // If not pressed, it's a key release.
    Key_Code key_code = KEY_UNKNOWN;

    Modifier_Flags modifier_flags;

    u32 utf32;              // If TEXT_INPUT event.
    b32 repeat = false;     // If KEYBOARD event.
    u16 text_input_count;   // If KEYBOARD event that also generated TEXT_INPUT events, this will tell you how many TEXT_INPUT events after this KEYBOARD event were generated.

    s32 typical_wheel_delta; // Used only for mouse events.
    s32 wheel_delta;         // Used only for mouse events.
};


// Thing (Discriminated Union)
//
enum OS_Thing_Kind : u8 {
    OS_THING_KIND_INVALID = 0,

    OS_THING_KIND_THREAD,

    OS_THING_KIND_COUNT
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
        return memcmp(u, other.u, sizeof(u)) != 0;
    }
};
global read_only const Guid NULL_GUID = {};


//
// OS State
//
struct OS_State {
    Allocator arena;

    // Platform-specific
    void *native;

    
    //
    // Input
    //
    Array<Event> events; // Events this frame
    Key_State input_button_states[KEY_CODE_MAX];
    b32 input_application_has_focus;

    // Per-frame mouse deltas:
    s64 mouse_delta_x;
    s64 mouse_delta_y;
    s64 mouse_delta_z;


    //
    // Thing
    //
    // Free list
    OS_Thing *first_free_thing;
    OS_Thing *last_free_thing;

    // Active list
    OS_Thing *first_thing[OS_THING_KIND_COUNT];
    OS_Thing *last_thing[OS_THING_KIND_COUNT];
};
extern OS_State *os;


// System
String get_path_of_running_executable(Allocator allocator);

// Initialize
void               os_init();

// Memory
void*              os_reserve(u64 size);
bool               os_commit(void* ptr, u64 size);
void               os_decommit(void* ptr, u64 size);
void               os_release(void* ptr, u64 size);
void*              os_heap_alloc(u64 size);
void               os_heap_free(void* ptr);

// System Info.
u32                os_query_core_count();
u32                os_query_page_size();
u32                os_query_caret_blink_time();

// Time
f64                time_seconds();
f64                time_ms();
f64                time_us();

// Handle Translation
bool               operator == (OS_Handle& l, OS_Handle& r);
OS_Handle          os_handle_from_hwnd(HWND hwnd);
OS_Handle          os_handle_from_win32_handle(HANDLE handle);
HWND               hwnd_from_os_handle(OS_Handle handle);
HANDLE             win32_handle_from_os_handle(OS_Handle handle);
void*              get_native_window_handle(OS_Handle window);


//
// File
//
struct File {
    HANDLE handle;
};

struct File_Visit_Info {
    String short_name;
    String full_name;

    // @Todo: Time
    s64    size;

    b32    had_error;
    b32    is_symlink;
    b32    is_directory;

    b32    descend_into_directory;
};

File file_open(String name, bool for_writing = false, bool keep_existing_content = false);

void file_close(File *file);

b32  file_move(String name_old, String name_new);

b32  file_copy(String src, String dst);

b32  file_delete(String name);

// Returns true on success.
b32 file_read(File file, void *vdata, s64 bytes_to_read);

// Returns length of the file. Returns -1 on error.
s64 file_length(File file);

// Returns the current file pointer offset. Returns -1 on error.
s64 file_current_position(File file);

// Sets file pointer offset. Returns false on error.
b32 file_set_position(File file, s64 pos);

String read_entire_file(File file, Allocator allocator, bool zero_terminated = false);

String read_entire_file(String name, Allocator allocator, bool zero_terminated = false);

bool file_write(File file, void *data, s64 size);

b32 file_is_valid(File file);

b32 delete_directory(String dirname);

b32 file_exists(String path);

b32 is_directory(String path);

b32 visit_files(String dir_name, 
                b32 recursive, 
                void *user_data, 
                void (*proc)(File_Visit_Info *, void *), 
                b32 follow_directory_symlinks = true,
                b32 visit_files = true,
                b32 visit_directories = false,
                b32 visit_symlinks = true);

Array<String> file_list(String path, 
                        Allocator allocator, 
                        b32 recursive = false, 
                        b32 follow_directory_symlinks = true);



// GFX
void               os_gfx_init();
OS_Handle          window_create(int w, int h, String name);
void               os_window_toggle_fullscreen(OS_Handle window);
vec2               os_window_size(OS_Handle window);
vec2               os_get_mouse_position(OS_Handle window);

// Input
void update_window_events();
void input_per_frame_event_and_flag_update();


// Mutex (non-re-entrant, meaning, 'lock -> lock' is invalid)
void               mutex_create(Mutex *mutex);
void               mutex_destroy(Mutex *mutex);
void               mutex_lock(Mutex *mutex);
void               mutex_unlock(Mutex *mutex);

// Condition Variable
void               condvar_create(Condvar *condvar);
void               condvar_destroy(Condvar *condvar);
Wait_Result        condvar_sleep(Condvar *condvar, Mutex *mutex, s64 timeout_ms);
void               condvar_wake_one(Condvar *condvar);
void               condvar_wake_all(Condvar *condvar);

// Semaphore
void               semaphore_create(Semaphore *semaphore);
void               semaphore_destroy(Semaphore *semaphore);
void               semaphore_signal(Semaphore *semaphore);
Wait_Result        semaphore_wait(Semaphore *semaphore, s32 milliseconds); // Pass in negative number to wait indefinitely.

// Thread
Thread             thread_launch(void (*proc)(void *), void *param);
bool               thread_join(Thread thread, s32 endt_us);
void               thread_set_name(String name);

// Thread Group
void               thread_group_init(Thread_Group *group, s32 num_threads, Arena *arena, String group_name);
void               thread_group_shutdown(Thread_Group *group);
void               thread_group_add_work(Thread_Group *group, void (*proc)(void *), void *param);
void               thread_group_complete_all_work(Thread_Group *group);

// UUID/GUID
Guid               guid_generate();
Guid               guid_from_bytes(void *bytes, u64 size);
Guid               guid_from_string(String str);

// Atomic
void               atomic_increment(volatile s32 *x);
void               atomic_store(volatile s32 *dst, s32 val);
void               atomic_store(volatile s64 *dst, s64 val);

template<typename F> 
void parallel_for(Thread_Group *group, s64 count, F&& func) {
    if (count <= 0)  return;

    s64 chunk_size = (count + group->count - 1) / group->count;

    struct Context {
        F  *func;
        s64 begin;
        s64 end;
    };

    auto proc = [](void *param) {
        Context *ctx = (Context *)param;

        for (s64 i = ctx->begin; i < ctx->end; ++i) {
            (*ctx->func)(i);
        }
    };

    for (s32 i = 0; i < group->count; ++i) {
        s64 begin = i * chunk_size;
        s64 end   = min(begin + chunk_size, count);

        if (begin >= end)  break;

        Context *ctx = push_struct(group->arena, Context);
        ctx->func  = &func;
        ctx->begin = begin;
        ctx->end   = end;

        thread_group_add_work(group, proc, ctx);
    }

    thread_group_complete_all_work(group);
}


#endif // OPERATING_SYSTEM_H
