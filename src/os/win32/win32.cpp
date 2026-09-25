// Copyright Seong Woo Lee. All Rights Reserved.

#include "os/os.h"
#include "basic/arena.h"
#include "basic/string.h"
#include "basic/context.h"
#include "basic/log.h"
#include "basic/hash_table.h"

#include <windowsx.h>
#include <hidusage.h>
#include <dbt.h>
#include <shlobj.h>

extern "C"
{
    __declspec(dllexport) DWORD NvOptimusEnablement = 1;
    __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}


OS_State *os;
f32 mouse_delta_x;
f32 mouse_delta_y;
f32 mouse_delta_z;

static b32 window_class_initted = false;
static Table<WPARAM, Key_Code> vk_to_key_code;
static Table<Key_Code, WPARAM> key_code_to_vk;
static Table<WPARAM, bool>     key_down_table;
static bool shift_state = false;
static bool ctrl_state  = false;
static bool alt_state   = false;
static Array<RAWINPUT> raw_input_buffer;


static struct VK_To_Key_Code {
    u32 vk;
    Key_Code key_code;
} vk_to_key_code_array[] = {
    { VK_LBUTTON,       MOUSE_BUTTON_LEFT },
    { VK_MBUTTON,       MOUSE_BUTTON_MIDDLE },
    { VK_RBUTTON,       MOUSE_BUTTON_RIGHT },
    { VK_SPACE,         (Key_Code)32 },
    { VK_HOME,          KEY_HOME },
    { VK_END,           KEY_END },
    { VK_PRIOR,         KEY_PAGE_UP },
    { VK_NEXT,          KEY_PAGE_DOWN },
    { VK_LEFT,          KEY_ARROW_LEFT },
    { VK_RIGHT,         KEY_ARROW_RIGHT },
    { VK_UP,            KEY_ARROW_UP },
    { VK_DOWN,          KEY_ARROW_DOWN },
    { VK_MENU,          KEY_ALT },
    { VK_SHIFT,         KEY_SHIFT },
    { VK_CONTROL,       KEY_CTRL },
    { VK_BACK,          KEY_BACKSPACE },
    { VK_DELETE,        KEY_DELETE },
    { VK_INSERT,        KEY_INSERT },
    { VK_ESCAPE,        KEY_ESCAPE },
    { VK_RETURN,        KEY_ENTER },
    { VK_TAB,           KEY_TAB },
    { VK_OEM_1,         (Key_Code)';' },
    { VK_OEM_2,         (Key_Code)'/' },
    { VK_OEM_3,         (Key_Code)'`' },
    { VK_OEM_4,         (Key_Code)'[' },
    { VK_OEM_5,         (Key_Code)'\\' },
    { VK_OEM_6,         (Key_Code)']' },
    { VK_OEM_7,         (Key_Code)'\'' },
    { VK_OEM_PLUS,      (Key_Code)'+' },
    { VK_OEM_MINUS,     (Key_Code)'-' },
    { VK_OEM_PERIOD,    (Key_Code)'.' },
    { VK_OEM_COMMA,     (Key_Code)',' },
    { VK_SNAPSHOT,      KEY_PRINT_SCREEN },
    { VK_PAUSE,         KEY_PAUSE },
    { VK_SCROLL,        KEY_SCROLL_LOCK },
};


//
// Utf
//
static String wide_to_utf8(u16 *data, s32 length, Allocator allocator) {
    if (length == 0)  return {};
    
    // length of -1 means it's zero-terminated.
    int query_result = WideCharToMultiByte(CP_UTF8, 0, (LPCWCH)data, length, NULL, 0, NULL, NULL);

    if (query_result <= 0)  return {};

    if (length != -1) {
        query_result += 1;
    }

    String name = {};
    u8 *name_bytes = alloc(sizeof(u8) * query_result, allocator);
    int result = WideCharToMultiByte(CP_UTF8, 0, (LPCWCH)data, length, (LPSTR)name_bytes, query_result, NULL, NULL);
    
    if (result <= 0) {
        return {};
    }

    R_ASSERT(result <= query_result);

    name.str = name_bytes;
    if (length == -1) {
        name.len = result - 1;
    } else {
        name.len = result;
        name.str[name.len] = 0;
    }

    return name;
}



//
// System
//
String get_executable_path(Allocator allocator) {
    u16 buf[MAX_PATH] = {};

    HMODULE my_handle = GetModuleHandleW(NULL);
    DWORD success = GetModuleFileNameW(my_handle, (LPWSTR)buf, MAX_PATH);

    if (success <= 0) {
        return {};
    } else {
        auto convert_slashes = [](String s) {
            for (s64 i = 0; i < s.len; ++i) {
                if (s.str[i] == '\\')  s.str[i] = '/';
            }
        };

        String exe_name = wide_to_utf8(buf, -1, allocator);
        convert_slashes(exe_name);

        return exe_name;
    }
}



//
// Initialize
//
static Key_Code get_key_code(WPARAM wParam) {
    auto t = table_find(&vk_to_key_code, wParam);
    if (t.found) return t.value;
    return KEY_UNKNOWN;
}

static u64 get_vk(Key_Code key) {
    auto t = table_find(&key_code_to_vk, key);
    if (t.found) return t.value;
    return 0;
}

static void init_key_code_tables() {
    auto add_code = [](WPARAM vk, Key_Code key_code) {
        table_add(&vk_to_key_code, vk, key_code);
        table_add(&key_code_to_vk, key_code, vk);
    };

    vk_to_key_code.allocator = os->arena;
    key_code_to_vk.allocator = os->arena;
    key_down_table.allocator = os->arena;

    // ASCII characters:
    for (u32 i = 48; i <= 90; ++i) {
        add_code(i, (Key_Code)i);
    }

    // Function keys:
    for (u32 i = VK_F1; i <= VK_F16; ++i) {
        u32 delta = i - VK_F1;
        add_code(i, (Key_Code)(KEY_F1 + delta));
    }

    // Numeric keypad:
    for (u32 i = VK_NUMPAD0; i <= VK_NUMPAD9; ++i) {
        u32 delta = i - VK_NUMPAD0;
        add_code(i, (Key_Code)((u32)'0' + delta));
    }

    // Entries defined by the array in this file:
    for (u32 i = 0; i < array_count(vk_to_key_code_array); ++i) {
        add_code(vk_to_key_code_array[i].vk, vk_to_key_code_array[i].key_code);
    }
}

void os_init() {
    // Bootstrap arena
    Allocator arena = arena_allocator_alloc();
    os = (OS_State*)alloc(sizeof(OS_State), arena);
    os->arena = arena;

    // Win32 State
    os->native = (Win32_State*)alloc(sizeof(Win32_State), arena);
    Win32_State *win32 = (Win32_State *)os->native;
    win32->window_arena = arena_alloc();

    // Events
    os->event_arena = arena_allocator_alloc();
    os->events.allocator = os->arena;

    init_key_code_tables();



    // Regsiter raw input. Follows keyboard focus
    RAWINPUTDEVICE rid[2] = {
        { HID_USAGE_PAGE_GENERIC, HID_USAGE_GENERIC_MOUSE,    0, NULL},
        { HID_USAGE_PAGE_GENERIC, HID_USAGE_GENERIC_KEYBOARD, 0, NULL},
    };

    if (!RegisterRawInputDevices(rid, 2, sizeof(RAWINPUTDEVICE))) {
        log_error(S("Failed to initialize raw input."));
        R_ASSERT(false);
    }

    raw_input_buffer.allocator = os->arena;
    array_reserve(&raw_input_buffer, 8000);

    { // Cache QPC frequency
        LARGE_INTEGER li;
        QueryPerformanceFrequency(&li);
        win32->qpc_frequency = li.QuadPart;
    }
}


//
// Thing
//
static OS_Thing *os_thing_alloc(OS_Thing_Kind kind) {
    OS_Thing *thing = os->first_free_thing;

    if (thing == NULL) {
        thing = (OS_Thing*)alloc(sizeof(OS_Thing), os->arena);
    } else {
        sll_pop_front(os->first_free_thing, os->last_free_thing);
        memset(thing, 0, sizeof(*thing));
    }

    thing->kind = kind;
    dll_push_back(os->first_thing[kind], os->last_thing[kind], thing);

    return thing;
}

static void os_thing_dealloc(OS_Thing *thing) {
    dll_remove(os->first_thing[thing->kind], os->last_thing[thing->kind], thing);
    sll_push_back(os->first_free_thing, os->last_free_thing, thing);
}


// Memory
//
void *os_reserve(u64 size) {
    void *result = VirtualAlloc(0, size, MEM_RESERVE, PAGE_READWRITE);
    return result;
}

bool os_commit(void* ptr, u64 size) {
    bool result = (VirtualAlloc(ptr, size, MEM_COMMIT, PAGE_READWRITE) != 0);
    // @Todo: RIORegisterBuffer()
    // Also, check out (https://github.com/cmuratori/largepages)
    // The problem with RIO is that now it cannot be evicted, so it's somewhat dangerous to ship.
    return result;
}

void os_decommit(void* ptr, u64 size) {
    VirtualFree(ptr, size, MEM_DECOMMIT);
}

void os_release(void* ptr, u64 size) {
    // Size isn't required on Windows, but is required on other OSes.
    VirtualFree(ptr, 0, MEM_RELEASE);
}

void *os_heap_alloc(u64 size) {
    return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
}

void os_heap_free(void* ptr) {
    HeapFree(GetProcessHeap(), 0, ptr);
}


// System Info.
//
u32 os_query_core_count() {
    SYSTEM_INFO info;
    GetSystemInfo(&info);
    return info.dwNumberOfProcessors;
}

u32 os_query_page_size() {
    SYSTEM_INFO info;
    GetSystemInfo(&info);
    return info.dwPageSize;
}

u32 os_query_caret_blink_time() {
    return GetCaretBlinkTime();
}


// Time
// @Todo: This is kinda sloppy...
f64 time_seconds() {
    Win32_State *win32 = (Win32_State *)os->native;
    LARGE_INTEGER li;
    QueryPerformanceCounter(&li);
    f64 seconds = (f64)li.QuadPart / (f64)win32->qpc_frequency;
    return seconds;
}

f64 time_ms() {
    return time_seconds() * 1000.0;
}

f64 time_us() {
    return time_seconds() * 1000000.0;
}


// Handle Translation
//
bool operator == (OS_Handle& l, OS_Handle& r) {
    return l.e[0] == r.e[0];
}

OS_Handle os_handle_from_hwnd(HWND hwnd) {
    OS_Handle result = {};
    result.e[0] = (u64)hwnd;
    return result;
}

OS_Handle os_handle_from_win32_handle(HANDLE handle) {
    OS_Handle result = {};
    result.e[0] = (u64)handle;
    return result;
}

HWND hwnd_from_os_handle(OS_Handle handle) {
    return (HWND)handle.e[0];
}

HANDLE win32_handle_from_os_handle(OS_Handle handle) {
    return (HANDLE)handle.e[0];
}


//
// File
//
File file_open(String name, bool for_writing, bool keep_existing_content) {
    HANDLE handle = {};
    LPCWSTR c_name = (LPCWSTR)to_utf16(tctx.temp, name).str;
    if (for_writing) {
        u32 mode = keep_existing_content ? OPEN_ALWAYS : CREATE_ALWAYS;
        handle = CreateFileW(c_name, FILE_GENERIC_READ | FILE_GENERIC_WRITE, FILE_SHARE_READ, NULL, mode, 0, NULL);
    } else {
        handle = CreateFileW(c_name, FILE_GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    }

    if (handle == INVALID_HANDLE_VALUE) {
        DWORD error_code = GetLastError();
        log(LOG_ERROR, S("Could not open file %S: code %d"), name, error_code);
        return {};
    }

    File file = {};
    file.handle = handle;

    return file;
}

void file_close(File *file) {
    if (file->handle != INVALID_HANDLE_VALUE) {
        CloseHandle(file->handle);
        file->handle = INVALID_HANDLE_VALUE;
    }
}

b32 file_move(String name_old, String name_new) {
    LPCWSTR c_name_old = (LPCWSTR)to_utf16(tctx.temp, name_old).str;
    LPCWSTR c_name_new = (LPCWSTR)to_utf16(tctx.temp, name_new).str;
    return MoveFileW(c_name_old, c_name_new);
}

b32 file_copy(String src, String dst) {
    LPCWSTR src16 = (LPCWSTR)to_utf16(tctx.temp, src).str;
    LPCWSTR dst16 = (LPCWSTR)to_utf16(tctx.temp, dst).str;

    BOOL success = CopyFileW(src16, dst16, 0);
    if ( success ) return false;
    return true;
}

b32 file_delete(String name) {
    LPCWSTR c_name = (LPCWSTR)to_utf16(tctx.temp, name).str;
    return DeleteFileW(c_name) != 0; // @Todo(swL): Error-message
}

// Handle must not have been opened with 'FILE_FLAG_OVERLAPPED' (async mode)
// More advanced handling of pipes is not supported.
b32 file_read(File file, void *vdata, s64 bytes_to_read) {
    u8 *data = (u8 *)vdata;

    if (bytes_to_read <= 0)  return false;
    if (data == NULL)        return false;

    s64 total_read = 0;

    while (total_read < bytes_to_read) {
        s64 remaining = bytes_to_read - total_read;
        DWORD to_read = 0;
        if (remaining <= 0x7fffffff) {
            to_read = (DWORD)remaining;
        } else {
            to_read = 0x7fffffff;
        }

        DWORD single_read_length = 0;
        BOOL read_success = ReadFile(file.handle, data + total_read, to_read, &single_read_length, NULL);
        total_read += single_read_length;
        if (!read_success) {
            return false;
        }

        if (single_read_length == 0) {
            return true;
        }
    }

    return true;
}

s64 file_length(File file) {
    LARGE_INTEGER size = {};
    if (GetFileSizeEx(file.handle, &size) == 0) {
        return -1;
    } else {
        return size.QuadPart;
    }
}

s64 file_current_position(File file) {
    LARGE_INTEGER li = {}, zero = {};
    if (SetFilePointerEx(file.handle, zero, &li, FILE_CURRENT) == 0) {
        return -1;
    } else {
        return li.QuadPart;
    }
}

b32 file_set_position(File file, s64 pos) {
    LARGE_INTEGER li = {};
    li.QuadPart = pos;
    if (SetFilePointerEx(file.handle, li, NULL, FILE_BEGIN) == 0){ 
        return false;
    } else {
        return true;
    }
}

String read_entire_file(File file, Allocator allocator, bool zero_terminated) {
    // @Todo: deallocation on fail?
    String s = {};

    LARGE_INTEGER size_struct = {};
    BOOL size_success = GetFileSizeEx(file.handle, &size_struct);
    if (!size_success)  return s;

    s64 length = size_struct.QuadPart;
    s64 zero_termination_size = 0;
    if (zero_terminated)  zero_termination_size = 1;

    u8 *data = (u8 *)alloc(length + zero_termination_size, allocator);
    if (data == NULL)  return s;

    DWORD single_read_length = 0;
    s64 total_read = 0;

    s64 previous_pos = file_current_position(file);
    if (previous_pos == -1)  return s;

    b32 set_success = file_set_position(file, 0);
    if (!set_success)  return s;

    while (total_read < length) {
        s64 remaining = length - total_read;
        DWORD to_read = 0;
        if (remaining <= 0x7fffffff) {
            to_read = (DWORD)remaining;
        } else {
            to_read = 0x7fffffff;
        }

        ReadFile(file.handle, data + total_read, to_read, &single_read_length, NULL);
        if (single_read_length <= 0) {
            dealloc(data, allocator);
            return s;
        }

        total_read += single_read_length;
    }

    s.len = length;
    s.str = data;

    if (zero_terminated)  s.str[length] = 0;

    if (!file_set_position(file, previous_pos)) {
        return s;
    }

    return s;
}

String read_entire_file(String name, Allocator allocator, bool zero_terminated) {
    String s = {};
    File file = file_open(name);
    if (file_is_valid(file)) {
        s = read_entire_file(file, allocator, zero_terminated);
        file_close(&file);
    }

    return s;
}

bool file_write(File file, void *data, s64 size) {
    // WritreFile maybe blocks unitl it writes everything, as long as the file it not set to nonblocking.
    // @Todo(swL): Deal with inputs > 32 bits.

    u32 size32 = (u32)size;
    R_ASSERT(size32 == size);

    DWORD written = 0;
    BOOL status = WriteFile(file.handle, data, size32, &written, NULL);

    return (bool)status;
}

b32 file_is_valid(File file) {
    return file.handle != NULL;
}

b32 file_exists(String path) {
    LPCWSTR c_name = (LPCWSTR)to_utf16(tctx.temp, path).str;
    return GetFileAttributesW(c_name) != INVALID_FILE_ATTRIBUTES;
}

b32 is_directory(String path) {
    LPCWSTR c_name = (LPCWSTR)to_utf16(tctx.temp, path).str;
    DWORD attrib = GetFileAttributesW(c_name);
    if (attrib == INVALID_FILE_ATTRIBUTES)  return false;

    b32 is_dir = ( attrib & FILE_ATTRIBUTE_DIRECTORY );
    return is_dir;
}

b32 visit_files(String _dir_name,
                b32 recursive,
                void *user_data,
                void (*proc)(File_Visit_Info *, void *),
                b32 follow_directory_symlinks,
                b32 visit_files,
                b32 visit_directories,
                b32 visit_symlinks)
{
    if ( !proc ) return true;

    Array<String> directories;
    directories.allocator = tctx.temp;

    array_add(&directories, _dir_name);

    File_Visit_Info info = {};

    s64 cursor = 0;
    while ( (u64)cursor < directories.count ) {
        String dir_name = directories[cursor];
        cursor += 1;

        u16 *wildcard_name = to_utf16(tctx.temp, tprint(S("%S/*"), dir_name)).str;

        WIN32_FIND_DATAW find_data;
        HANDLE handle = FindFirstFileExW((LPWSTR)wildcard_name, FindExInfoBasic, &find_data, FindExSearchNameMatch, NULL, FIND_FIRST_EX_LARGE_FETCH);

        if ( handle == INVALID_HANDLE_VALUE ) {
            if ( cursor != 1 ) continue;
            log_error(S("Unable to open directory: '%S'"), dir_name);
            return false;
        }

        while(1) {
            String name      = wide_to_utf8((u16*)find_data.cFileName, -1, tctx.temp);
            String full_name = tprint(S("%S/%S"), dir_name, name);

            info.short_name             = name;
            info.full_name              = full_name;
            info.descend_into_directory = false;
            info.size                   = ( (s64)find_data.nFileSizeHigh ) << 32 | find_data.nFileSizeLow;

            info.is_symlink             = (find_data.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) && (find_data.dwReserved0 == IO_REPARSE_TAG_SYMLINK);
            info.is_directory           = (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;

            if ( info.is_directory ) {
                if ( name != S(".") && name != S("..") ) {
                    info.descend_into_directory = recursive && (follow_directory_symlinks || !info.is_symlink);
                    if ( visit_directories && (visit_symlinks || !info.is_symlink) ) {
                        // info.modification_time = ;
                        proc(&info, user_data);
                    }

                    if ( info.descend_into_directory )  array_add(&directories, full_name);
                }
            } else {
                if ( visit_files && (visit_symlinks || !info.is_symlink) ) {
                    // info.modification_time = ;
                    proc(&info, user_data);
                }
            }

            BOOL success = FindNextFileW(handle, &find_data);
            if ( !success ) break;
        }

        FindClose(handle);
    }

    return true;
}

//
// GFX
//
static Win32_Window *win32_window_from_handle(OS_Handle handle) {
    Win32_State *state = (Win32_State *)os->native;
    HWND hwnd = hwnd_from_os_handle(handle);

    list_for (state->window_first, it) {
        if (it->handle == hwnd) return it;
    }

    return NULL;
}

static Win32_Window *win32_window_alloc() {
    Win32_State *state = (Win32_State *)os->native;
    Win32_Window *window = state->window_free_first;

    if (window == NULL) {
        window = push_struct(state->window_arena, Win32_Window);
    } else {
        sll_pop_front(state->window_free_first, state->window_free_last);
        memset(window, 0, sizeof(*window));
    }

    dll_push_back(state->window_first, state->window_last, window);

    return window;
}

static bool set_key_down_state(WPARAM vkey, bool is_down) {
    bool was_down = (table_find_pointer(&key_down_table, vkey) != NULL);
    if (is_down && !was_down) {
        table_add(&key_down_table, vkey, true);
    } else if (was_down && !is_down) {
        table_remove(&key_down_table, vkey);
    }

    return was_down;
}

static void send_key_event(Key_Code key_code, bool key_down, bool repeat = false) {
    if (key_code == KEY_ALT)    alt_state   = key_down;
    if (key_code == KEY_SHIFT)  shift_state = key_down;
    if (key_code == KEY_CTRL)   ctrl_state  = key_down;

    Event event = {};
    event.type = EVENT_KEYBOARD;
    event.key_pressed = key_down;
    event.key_code = key_code;
    event.modifier_flags.packed = 0;
    event.modifier_flags.shift_pressed = shift_state;
    event.modifier_flags.ctrl_pressed  = ctrl_state;
    event.modifier_flags.alt_pressed   = alt_state;
    event.repeat = repeat;
    array_add(&os->events, event);

    os->input_button_states[key_code] |= ( key_down ? (KEY_STATE_DOWN | KEY_STATE_START) : KEY_STATE_END) ;
}

static void maybe_send_vkey_event(u64 vkey, bool key_down, bool repeat = false) {
    bool was_down = set_key_down_state((u32)vkey, key_down);
    if (!key_down && !was_down) {
        // redundant key_up event
        return;
    }

    if (key_down && repeat && !was_down) {
        // key was pressed while we didn't have focus so the first 
        // event we see is incorrectly labeled as a repeat.
        repeat = false;
    }

    send_key_event(get_key_code(vkey), key_down, repeat);
}

static void process_raw_input(HRAWINPUT handle) {
    UINT dw_size = 0;
    GetRawInputData(handle, RID_INPUT, NULL, &dw_size, sizeof(RAWINPUTHEADER));

    UINT written_bytes = GetRawInputData(handle, RID_INPUT, raw_input_buffer.data, &dw_size, sizeof(RAWINPUTHEADER));
    if (written_bytes == 0xffffffff) return;
    R_ASSERT(written_bytes <= dw_size);

    RAWINPUT *raw = (RAWINPUT*)raw_input_buffer.data;

    if (raw->header.dwType == RIM_TYPEMOUSE) {
        RAWMOUSE *mouse = &raw->data.mouse;

        if (mouse->usFlags & MOUSE_MOVE_ABSOLUTE) {
            // @Todo: Seems bogus
        } else {
            mouse_delta_x += mouse->lLastX;
            mouse_delta_y += mouse->lLastY;
        }
    } else if (raw->header.dwType == RIM_TYPEKEYBOARD) {
        // @Todo: if I ever want print screen key.
    }
}

LRESULT RtsWindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    LRESULT result = 0;

    switch(msg) 
    {
        case WM_SYSCOMMAND:
        return DefWindowProcW(hwnd, msg, wparam, lparam);

        case WM_ACTIVATEAPP: {
            if (wparam) { // We are being activated.

            }

            return DefWindowProcW(hwnd, msg, wparam, lparam);
        }

        case WM_SYSKEYDOWN:
        case WM_KEYDOWN: {

            bool repeat = (((s32)lparam) & 0x40000000) != 0;
            maybe_send_vkey_event(wparam, true, repeat);

        } break;

        case WM_SYSKEYUP:
        case WM_KEYUP: {

            maybe_send_vkey_event(wparam, false);

        } break;


        case WM_SYSCHAR: 
        // This is here to prevent beeps when a Alt key combo is pressed. if we don't return 0, 
        // windows helpfully emits a beep sound to indicate the user that the key wasn't handled.
        break;

        case WM_CHAR: 
        {
            WPARAM keycode = wparam;

            if (keycode > 31) {
                Event event = {};
                event.type = EVENT_TEXT_INPUT;
                event.utf32 = keycode;

                array_add(&os->events, event);
            }
        } break;

        case WM_SETFOCUS: 
        os->input_application_has_focus = true;
        break;

        case WM_KILLFOCUS: 
        os->input_application_has_focus = false;
        break;

        case WM_PAINT: 
        {
            // Windows maintains lists of "dirty" triangles that window must redraw.
            // Then it calls WM_PAINT and expectation is that window redraws those regions.
            // Classically you did that with BeginPaint/EndPaint and GDI, but as we are on 
            // Direct3D, calling ValidateRect with NULL simply says that whole window 
            // region is valid. No need to worry about individual dirty rectangles.
            ValidateRect(hwnd, NULL);
            return DefWindowProcW(hwnd, msg, wparam, lparam);
        }

        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        {
            maybe_send_vkey_event(VK_LBUTTON, msg == WM_LBUTTONDOWN);

            if (msg == WM_LBUTTONDOWN) SetCapture(hwnd);
            else ReleaseCapture();
        } break;

        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
        {
            maybe_send_vkey_event(VK_RBUTTON, msg == WM_RBUTTONDOWN);
        } break;

        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP:
        {
            maybe_send_vkey_event(VK_MBUTTON, msg == WM_MBUTTONDOWN);
        } break;


        case WM_MOUSEMOVE:
        {
#if 0
            f32 x = (f32)(s16)LOWORD(lparam);
            f32 y = (f32)(s16)HIWORD(lparam);
            vec2 position = vec2{x, y};

            OS_Event* event = os_push_event();
            event->kind     = OS_EVENT_MOUSE_MOVE;
            event->window   = window_handle;
            event->position = position;
#endif
        } break;


        case WM_MOUSEWHEEL:
        case WM_MOUSEHWHEEL:
        {
            Event event = {};
            event.type = EVENT_MOUSE_WHEEL;
            event.typical_wheel_delta = WHEEL_DELTA;
            event.wheel_delta = (s16)(wparam >> 16);
            array_add(&os->events, event);

            mouse_delta_z += event.wheel_delta;
        } break;


        case WM_CLOSE: 
        case WM_QUIT:
        {
            Event event = {};
            event.type = EVENT_QUIT;
            array_add(&os->events, event);
            return DefWindowProcW(hwnd, msg, wparam, lparam);
        }

        
        case WM_INPUT:
        {
            LPARAM extra = GetMessageExtraInfo();
            if ((extra & 0x82) == 0x82) {
                // @Ignore touch input.
            } else {
                process_raw_input((HRAWINPUT)lparam);
            }
            DefWindowProcW(hwnd, msg, wparam, lparam);
        } break;

        case WM_DROPFILES:
        {
            HDROP drop = (HDROP)wparam;
            UINT num_files = DragQueryFileW(drop, 0xFFFFFFFF, NULL, 0);
            R_ASSERT(num_files > 0);

            Array<String> files;
            files.allocator = os->event_arena;

            for (UINT i = 0; i < num_files; ++i) {
                UINT n = DragQueryFileW(drop, i, NULL, 0) + 2;

                u16 *filename_wide = (u16*)alloc(n * sizeof(u16), os->event_arena);
                UINT success = DragQueryFileW(drop, i, (LPWSTR)filename_wide, n);
                R_ASSERT(success);

                String filename = wide_to_utf8(filename_wide, n, os->event_arena);
                array_add(&files, filename);
            }

            DragFinish(drop);

            Event event;
            event.type  = EVENT_DRAG_AND_DROP_FILES;
            event.files = files;
            array_add(&os->events, event);

            return 0;
        }

        case WM_SIZE: {
            if (wparam == SIZE_MAXIMIZED) {

            } else if (wparam == SIZE_RESTORED)  {

            } else if (wparam == SIZE_MINIMIZED) {

            }
            return 0;
        }

        case WM_MOVE:
        return 0;

        case WM_EXITSIZEMOVE:
        return 0;

        case WM_DPICHANGED:
        return 0;

        case WM_CAPTURECHANGED:
        return 0;

        default:
        return DefWindowProcW(hwnd, msg, wparam, lparam);
    }

    return result;
}

void update_window_events() {
    input_per_frame_event_and_flag_update();

    // GetAsyncKeyState actually checks the key, not to be confused with GetKeyState, which does nothing.
    if (alt_state || (os->input_button_states[KEY_ALT] & KEY_STATE_DOWN)) {
        SHORT state = GetAsyncKeyState(VK_MENU);
        if (!(state & 0x8000)) {
            alt_state = false;
            os->input_button_states[KEY_ALT] |= KEY_STATE_END;
        }
    }

    if (ctrl_state || (os->input_button_states[KEY_CTRL] & KEY_STATE_DOWN)) {
        SHORT state = GetAsyncKeyState(VK_CONTROL);
        if (!(state & 0x8000)) {
            ctrl_state = false;
            os->input_button_states[KEY_CTRL] |= KEY_STATE_END;
        }
    }

    if (shift_state || (os->input_button_states[KEY_SHIFT] & KEY_STATE_DOWN)) {
        SHORT state = GetAsyncKeyState(VK_SHIFT);
        if (!(state & 0x8000)) {
            shift_state = false;
            os->input_button_states[KEY_SHIFT] |= KEY_STATE_END;
        }
    }

    for (;;) {
        MSG msg = {};

        BOOL result = PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE);
        if (!result)  break;

        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}

void os_window_dealloc(OS_Handle handle) {
    Win32_State *state = (Win32_State *)os->native;
    Win32_Window *window = win32_window_from_handle(handle);
    if (window) {
        dll_remove(state->window_first, state->window_last, window);
        sll_push_back(state->window_free_first, state->window_free_last, window);
    }
}

static void win32_init_window_class() {
    HINSTANCE hinst = GetModuleHandleW(0);

    WNDCLASSEXW wcex = {};
    {
        wcex.cbSize         = sizeof(wcex);
        wcex.style          = CS_HREDRAW | CS_VREDRAW;
        wcex.lpfnWndProc    = RtsWindowProc;
        wcex.hInstance      = hinst;
        wcex.hIcon          = LoadIconW(hinst, L"Icon");
        wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
        wcex.hbrBackground  = CreateSolidBrush(RGB(30, 20, 20));
        wcex.lpszClassName  = L"RtsWindowClass";
    }
    RegisterClassExW(&wcex);

    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
}

OS_Handle window_create(int w, int h, String name, b32 drag_accept_files) {
    if (!window_class_initted) {
        win32_init_window_class();
        window_class_initted = true;
    }

    HINSTANCE hinst = GetModuleHandleW(0);

    HWND hwnd = CreateWindowExW(NULL, 
                                L"RtsWindowClass", 
                                (LPCWSTR)to_utf16(tctx.temp, name).str, 
                                WS_OVERLAPPEDWINDOW, 
                                CW_USEDEFAULT, 
                                CW_USEDEFAULT, 
                                w, h, 
                                0, NULL, hinst, NULL);

    DragAcceptFiles(hwnd, drag_accept_files);

    UpdateWindow(hwnd);
    ShowWindow(hwnd, SW_SHOW);

    auto *window = win32_window_alloc();
    window->handle = hwnd;
    window->placement = { sizeof(window->placement) };

    OS_Handle handle = os_handle_from_hwnd(hwnd);
    return handle;
}

//
// Thanks, Raymond Chen.
// (https://devblogs.microsoft.com/oldnewthing/20100412-00/?p=14353)
//
void toggle_fullscreen(OS_Handle window_handle) 
{
    Win32_Window *window = win32_window_from_handle(window_handle);
    if (window) 
    {
        HWND hwnd = window->handle;
        DWORD dwStyle = GetWindowLong(hwnd, GWL_STYLE);
        if (dwStyle & WS_OVERLAPPEDWINDOW) 
        {
            MONITORINFO mi = { sizeof(mi) };
            if (GetWindowPlacement(hwnd, &window->placement) &&
                GetMonitorInfo(MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST), &mi)) 
            {
                SetWindowLong(hwnd, GWL_STYLE,
                              dwStyle & ~WS_OVERLAPPEDWINDOW);
                SetWindowPos(hwnd, HWND_TOP,
                             mi.rcMonitor.left, mi.rcMonitor.top,
                             mi.rcMonitor.right - mi.rcMonitor.left,
                             mi.rcMonitor.bottom - mi.rcMonitor.top,
                             SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
            }
        } else {
            SetWindowLong(hwnd, GWL_STYLE,
                          dwStyle | WS_OVERLAPPEDWINDOW);
            SetWindowPlacement(hwnd, &window->placement);
            SetWindowPos(hwnd, NULL, 0, 0, 0, 0,
                         SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
                         SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
        }
    }
}

Triplet<u32,u32,b32> window_size(OS_Handle window) {
    HWND hwnd = hwnd_from_os_handle(window);
    RECT rect;
    BOOL success = GetClientRect(hwnd, &rect);
    if (!success) {
        return {0,0,false};
    }
    u32 x = rect.right  - rect.left;
    u32 y = rect.bottom - rect.top;
    return {x,y,true};
}

static Triplet<s64,s64,b32> _get_mouse_pointer_position(HWND hwnd) {
    POINT p = {};
    BOOL ok = GetCursorPos(&p);
    if (!ok) {
        return {0,0,false};
    }

    ok = ScreenToClient(hwnd, &p);
    if (!ok) {
        return {0,0,false};
    }

    return {p.x, p.y, true};
}

Triplet<s64,s64,b32> get_mouse_pointer_position(OS_Handle window) {
    HWND hwnd = hwnd_from_os_handle(window);
    return _get_mouse_pointer_position(hwnd);
}

void* get_native_window_handle(OS_Handle window) {
    return (void *)hwnd_from_os_handle(window);
}



// Mutex
//
// Do not use CriticalSection on Windows. It is legacy API. Use SRWLock for
// much simpler and better API for mutex. If you want to be very fancy they you
// can use futex'es - WaitOnAddress & its friends, but that will require much
// more carefully written code. For basic lock/unlock/condvar, SRWLock is way
// to go. SRWLock is not re-entrant, but just don't do it.
//
void mutex_create(Mutex *mutex) {
    InitializeSRWLock(&mutex->lock);
}

void mutex_destroy(Mutex *mutex) {
    // no-op
}

void mutex_lock(Mutex *mutex) {
    AcquireSRWLockExclusive(&mutex->lock);
}

void mutex_unlock(Mutex *mutex) {
    ReleaseSRWLockExclusive(&mutex->lock);
}


// Condition Variable
//
void condvar_create(Condvar *condvar) {
    InitializeConditionVariable(&condvar->var);
}

void condvar_destroy(Condvar *condvar) {
    // no-op
}

Wait_Result condvar_sleep(Condvar *condvar, Mutex *mutex, s64 timeout_ms) {
    DWORD ms = timeout_ms == -1 ? INFINITE : (DWORD)timeout_ms;
    BOOL res = SleepConditionVariableSRW(&condvar->var, &mutex->lock, ms, 0);
    if (res == 0) {
        if (GetLastError() == ERROR_TIMEOUT) return WAIT_RESULT_TIMEOUT;
        else return WAIT_RESULT_ERROR; 
    }
    return WAIT_RESULT_SUCCESS;
}

void condvar_wake_one(Condvar *condvar) {
    WakeConditionVariable(&condvar->var);
}

void condvar_wake_all(Condvar *condvar) {
    WakeAllConditionVariable(&condvar->var);
}


// Semaphore
//
// Windows Semaphore involves system call, thus any operation on it incurs
// roundtrip through kernel. Mutex and ConditionVariable have fast paths. They
// first do super cheap atomic op to check if they can lock and take ownership.
// And only if they cannot they do the syscall to kernel. So unless you really
// need semaphore for some reason, mutex + condar is way to go.
//
void semaphore_create(Semaphore *semaphore) {
    semaphore->event = CreateSemaphore(NULL, 0, 0x7fffffff, NULL);
}

void semaphore_destroy(Semaphore *semaphore) {
    CloseHandle(semaphore->event);
}

void semaphore_signal(Semaphore *semaphore) {
    ReleaseSemaphore(semaphore->event, 1, NULL);
}

Wait_Result semaphore_wait(Semaphore *semaphore, s32 milliseconds) {
    DWORD res = S_OK;

    if (milliseconds < 0) {
        res = WaitForSingleObject(semaphore->event, INFINITE);
    } else {
        res = WaitForSingleObject(semaphore->event, (u32)milliseconds);
    }

    if (res == WAIT_OBJECT_0)  return WAIT_RESULT_SUCCESS;
    if (res == WAIT_TIMEOUT)   return WAIT_RESULT_TIMEOUT;
    else                       return WAIT_RESULT_ERROR;
}


//
// Thread
//
static DWORD _win32_thread_entry(void *ptr) {
    thread_init();

    OS_Thing *thing = (OS_Thing *)ptr;

    auto *proc  = thing->thread.proc;
    void *param = thing->thread.param;

    proc(param);

    return 0;
}

Thread thread_launch(void (*proc)(void *), void *param) {
    OS_Thing_Kind kind = OS_THING_KIND_THREAD;
    OS_Thing *thing = os_thing_alloc(kind);

    Thread *thread = &thing->thread;

    thread->proc  = proc;
    thread->param = param;

    HANDLE handle = CreateThread(NULL, 0, _win32_thread_entry, thing, 0, (DWORD *)&thread->tid);

    if (handle) {
        thread->handle = os_handle_from_win32_handle(handle); 
    }

    return *thread;
}

bool thread_join(Thread thread, s32 milliseconds) {
    R_ASSERT(milliseconds == -1); // @Temporary
    DWORD timeout = INFINITE;

    OS_Thing *thing = NULL;

    list_for(os->first_thing[OS_THING_KIND_THREAD], it) {
        auto *t = &it->thread;
        if (t->handle == thread.handle && t->tid == thread.tid) {
            thing = it;
            break;
        }
    }

    DWORD wait_result = WAIT_OBJECT_0;

    if (thing) {
        HANDLE handle = win32_handle_from_os_handle(thing->thread.handle);
        wait_result = WaitForSingleObject(handle, timeout);
        CloseHandle(handle);

        os_thing_dealloc(thing);
    }

    return wait_result == WAIT_OBJECT_0;
}

void thread_set_name(String name) {
    Utf16 name_16 = to_utf16(tctx.temp, name);

    // Minimum supported client	Windows 10, version 1607
    SetThreadDescription(GetCurrentThread(), (WCHAR *)name_16.str);
}


//
// Thread Group
//
static void work_list_init(Work_List *list) {
    memset(list, 0, sizeof(*list));
    semaphore_create(&list->semaphore);
    mutex_create(&list->mutex);
}

static void work_list_deinit(Work_List *list) {
    semaphore_destroy(&list->semaphore);
    mutex_destroy(&list->mutex);
}

static void _add_work(Work_List *list, Work_Entry *entry) {
    mutex_lock(&list->mutex);
    {
        if (list->last) {
            list->last->next = entry;
            list->last       = entry;
        } else {
            list->first      = entry;
            list->last       = entry;
        }

        list->count += 1;
    }
    mutex_unlock(&list->mutex);

    semaphore_signal(&list->semaphore);
}

static Work_Entry *_get_work(Work_List *list) {
    Work_Entry* result = NULL;

    mutex_lock(&list->mutex);
    {
        result = list->first;

        if (result) {
            list->first = result->next;

            if (!list->first) {
                list->last = NULL;
            }

            result->next = NULL;

            list->count -= 1;
        }
    }
    mutex_unlock(&list->mutex);

    return result;
}

static void _worker_info_init(Worker_Info *info, Thread_Group *group, s32 index) {
    work_list_init(&info->available);

    info->group = group;
    info->index = index;
}

static void _worker_info_deinit(Worker_Info *info) {
    work_list_deinit(&info->available);

    info->group = NULL;
    info->index = 0;
}

static void _thread_group_proc(void *param) {
    Worker_Info *info   = (Worker_Info *)param;
    Thread_Group *group = info->group;
    Work_List *list     = &info->available;

    if (group->name.str && group->name.len) {
        String name = tprint(S("%S_%d"), group->name, info->index);
        thread_set_name(name);
    }

    Work_Entry *entry = NULL;

    while (!group->should_shutdown) {
        if (!entry) {
            semaphore_wait(&list->semaphore, -1);

            if (group->should_shutdown) break; // abort immediately.

            entry = _get_work(list);
        }

        if (entry) {
            entry->thread_index = info->index;
            entry->proc(entry->param);

            InterlockedIncrement64(&group->completed);
        }

        entry = NULL;
    }
}

void thread_group_init(Thread_Group *group, s32 num_threads, Arena *arena, String group_name) {
    group->should_shutdown = false;
    group->arena           = arena;
    group->count           = num_threads;
    group->worker_info     = push_array_aligned(group->arena, Worker_Info, num_threads, CACHE_LINE_SIZE); // No false-sharing.

    if (group_name.str && group_name.len) {
        group->name = group_name;
    }

    for (s32 i = 0; i < num_threads; ++i) {
        auto *info = &group->worker_info[i];
        _worker_info_init(info, group, i);

        info->thread = thread_launch(_thread_group_proc, info);
    }

    group->temp    = temporary_arena_begin(group->arena);
    group->initted = true;
}

void thread_group_shutdown(Thread_Group *group) {
    // Should exit "properly": never use 'ExitThread()' or 'TerminateThread()'.
    R_ASSERT(group->initted);

    group->should_shutdown = true;

    for (s32 i = 0; i < group->count; ++i) {
        semaphore_signal(&group->worker_info[i].available.semaphore);
    }

    for (s32 i = 0; i < group->count; ++i) {
        thread_join(group->worker_info[i].thread, -1);
    }

    for (s32 i = 0; i < group->count; ++i) {
        auto *info = &group->worker_info[i];
        _worker_info_deinit(info);
    }
}

void thread_group_add_work(Thread_Group *group, void (*proc)(void *), void *param) {
    auto *entry = push_struct(group->arena, Work_Entry);

    entry->proc  = proc;
    entry->param = param;

    s32 index = group->next_worker_index++;
    group->next_worker_index %= group->count;

    InterlockedIncrement64(&group->added);

    _add_work(&group->worker_info[index].available, entry);
}

void thread_group_complete_all_work(Thread_Group *group) {
    while (group->completed != group->added) {
        _mm_pause(); // spin hint to the processor
    }

    group->completed = 0;
    group->added     = 0;

    // Flush allocated work entries.
    temporary_arena_end(group->temp);
    group->temp = temporary_arena_begin(group->arena);
}

//
// UUID/GUID
//
Guid guid_generate() {
    Guid result = {};
    UUID uuid;
    RPC_STATUS status = UuidCreate(&uuid);
    if (status == RPC_S_OK) {
        result.data1 = uuid.Data1;
        result.data2 = uuid.Data2;
        result.data3 = uuid.Data3;
        memcpy(result.data4, uuid.Data4, 8);
    }

    if (result == NULL_GUID) {
        result.u[15] += 1;
    }

    return result;
}


// Atomic
//
void atomic_increment(volatile s32 *x) {
    InterlockedIncrement((LONG *)x);
}

void atomic_store(volatile s32 *dst, s32 val) {
    InterlockedExchange((LONG *)dst, val);
}

void atomic_store(volatile s64 *dst, s64 val) {
    InterlockedExchange64(dst, val);
}

//
//
b32 set_working_directory(String s) {
    Utf16 wide = to_utf16(tctx.temp, s);
    if (!wide.str) return false;

    BOOL result = SetCurrentDirectoryW((LPWSTR)wide.str);
    return result;
}


// Main Entry
//
#if !BUILD_NO_ENTRY
int win32_main_entry() {
    os_init();
    thread_init();

    return main_entry(0, NULL);
}

/*
int wWinMain(HINSTANCE hinst, HINSTANCE deprecated, PWSTR cmd, int show_cmd) {
    return win32_main_entry();
}
*/

int main(int argc, char **argv) {
    return win32_main_entry();
}
#endif


// Utilities
//
String string_from_hresult(HRESULT hr) {

#define X(code) \
    case code: \
        return S(#code);

    switch (hr) {
        // Common
        X(S_OK);
        X(E_ABORT);
        X(E_ACCESSDENIED);
        X(E_FAIL);
        X(E_HANDLE);
        X(E_INVALIDARG);
        X(E_NOINTERFACE);
        X(E_NOTIMPL);
        X(E_OUTOFMEMORY);
        X(E_POINTER);

        // DXGI
        X(DXGI_ERROR_ACCESS_DENIED)
        X(DXGI_ERROR_ACCESS_LOST)
        X(DXGI_ERROR_ALREADY_EXISTS)
        X(DXGI_ERROR_CANNOT_PROTECT_CONTENT)
        X(DXGI_ERROR_DEVICE_HUNG)
        X(DXGI_ERROR_DEVICE_REMOVED)
        X(DXGI_ERROR_DEVICE_RESET)
        X(DXGI_ERROR_DRIVER_INTERNAL_ERROR)
        X(DXGI_ERROR_FRAME_STATISTICS_DISJOINT)
        X(DXGI_ERROR_GRAPHICS_VIDPN_SOURCE_IN_USE)
        X(DXGI_ERROR_INVALID_CALL)
        X(DXGI_ERROR_MORE_DATA)
        X(DXGI_ERROR_NAME_ALREADY_EXISTS)
        X(DXGI_ERROR_NONEXCLUSIVE)
        X(DXGI_ERROR_NOT_CURRENTLY_AVAILABLE)
        X(DXGI_ERROR_NOT_FOUND)
        X(DXGI_ERROR_REMOTE_CLIENT_DISCONNECTED)
        X(DXGI_ERROR_REMOTE_OUTOFMEMORY)
        X(DXGI_ERROR_RESTRICT_TO_OUTPUT_STALE)
        X(DXGI_ERROR_SDK_COMPONENT_MISSING)
        X(DXGI_ERROR_SESSION_DISCONNECTED)
        X(DXGI_ERROR_UNSUPPORTED)
        X(DXGI_ERROR_WAIT_TIMEOUT)
        X(DXGI_ERROR_WAS_STILL_DRAWING)

        default: return S("N/A");
    }
#undef X
}
