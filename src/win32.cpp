/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Seong Woo Lee $
   $Notice: (C) Copyright %s by Seong Woo Lee. All Rights Reserved. $
   ======================================================================== */

#define NOMINMAX
#define UNICODE
#define _UNICODE
#include <windows.h>
#include <Xinput.h>
#include <xaudio2.h>

#pragma comment(lib, "user32")
#pragma comment(lib, "gdi32")
#pragma comment(lib, "winmm")
#pragma comment(lib, "ole32")

// -----------------------------------
// @Note: [.h]
#include "base/rts_base_inc.h"
#include "rts_math.h"
#include "rts_asset.h"

#include "input.h"
#include "os/rts_os.h"

#include "rts_platform.h"
#include "win32.h"

// -----------------------------------
// @Note: [.cpp]
#include "base/rts_base_inc.cpp"
#include "rts_math.cpp"
#include "os/rts_os.cpp"

#include "win32_xinput.cpp"
#include "win32_keycode.cpp"

global Win32_State          g_win32_state;
global b32                  g_running = true;
global b32                  g_show_cursor = true;
global WINDOWPLACEMENT      g_window_placement = {sizeof(g_window_placement)};

#include "renderer.h"
#include "win32_renderer.h"


internal void
win32_get_exe_filepath(Win32_State *state)
{
    DWORD size_of_filename = GetModuleFileNameA(0, state->exe_filepath, sizeof(state->exe_filepath));
    state->one_past_last_exe_filepath_slash = state->exe_filepath;
    for (char *scan = state->exe_filepath; *scan; ++scan) {
        if (*scan == '\\') {
            state->one_past_last_exe_filepath_slash = (scan + 1);
        }
    }
}

internal void 
win32_toggle_fullscreen(HWND window)
{
    // @Note: In courtesy of Raymond Chen.
    DWORD style = GetWindowLong(window, GWL_STYLE);
    if (style & WS_OVERLAPPEDWINDOW) {
        MONITORINFO mi = { sizeof(mi) };
        if (GetWindowPlacement(window, &g_window_placement) &&
            GetMonitorInfo(MonitorFromWindow(window, MONITOR_DEFAULTTOPRIMARY), &mi)) 
        {
            SetWindowLong(window, GWL_STYLE,
                          style & ~WS_OVERLAPPEDWINDOW);
            SetWindowPos(window, HWND_TOP,
                         mi.rcMonitor.left, mi.rcMonitor.top,
                         mi.rcMonitor.right - mi.rcMonitor.left,
                         mi.rcMonitor.bottom - mi.rcMonitor.top,
                         SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
        }
    } else {
        SetWindowLong(window, GWL_STYLE, style | WS_OVERLAPPEDWINDOW);
        SetWindowPlacement(window, &g_window_placement);
        SetWindowPos(window, 0, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
                     SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    }
}

internal v2u 
win32_get_window_dimension(HWND hwnd) 
{
    v2u result = {};
    RECT rect;
    GetClientRect(hwnd, &rect);
    result.w = (u32)(rect.right - rect.left);
    result.h = (u32)(rect.bottom - rect.top);
    return result;
}

internal FILETIME
win32_get_filetime(LPWSTR filename) 
{
    FILETIME result = {};
    WIN32_FIND_DATA find_data;
    FindFirstFileW(filename, &find_data);
    result = find_data.ftLastWriteTime;

    return result;
}

internal void
win32_xinput_handle_deadzone(XINPUT_STATE *state) 
{
#define XINPUT_DEAD_ZONE 2500
    f32 lx = state->Gamepad.sThumbLX;
    f32 ly = state->Gamepad.sThumbLY;

    if (sqrt(lx*lx + ly*ly) < XINPUT_DEAD_ZONE) {
        state->Gamepad.sThumbLX = 0;
        state->Gamepad.sThumbLY = 0;
    }
    f32 rx = state->Gamepad.sThumbRX;
    f32 ry = state->Gamepad.sThumbRY;
    if (sqrt(rx*rx + ry*ry) < XINPUT_DEAD_ZONE) {
        state->Gamepad.sThumbRX = 0;
        state->Gamepad.sThumbRY = 0;
    }
}

internal HANDLE
win32_get_file_handle(Os_File_Handle *handle)
{
    return *(HANDLE *)&handle->platform;
}

internal void
win32_process_keyboard(Game_Key *game_key, b32 is_down) 
{
    if (is_down) game_key->is_down = true; 
    else game_key->is_down = false;
}

internal void
win32_process_mouse_click(s32 vk, Mouse_Input *mouse) 
{
    b32 is_down = GetKeyState(vk) & (1 << 15);
    u32 E = 0;

    switch(vk) 
    {
        case VK_LBUTTON: 
        {
            E = Mouse_Left;
        } break;
        case VK_MBUTTON: 
        {
            E = Mouse_Middle;
        } break;
        case VK_RBUTTON: 
        {
            E = Mouse_Right;
        } break;

        INVALID_DEFAULT_CASE;
    }

    if (is_down) 
    {
        if (!mouse->is_down[E]) 
        {
            mouse->toggle[E] = true;
            mouse->click_p[E] = mouse->position;
        } 
        else 
        {
            mouse->toggle[E] = false;
        }
    } 
    else 
    {
        if (mouse->is_down[E]) 
        {
            mouse->toggle[E] = true;
        } 
        else 
        {
            mouse->toggle[E] = false;
        }
    }
    mouse->is_down[E] = is_down;
}

internal LRESULT 
win32_window_callback(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) 
{
    LRESULT result = 0;

    switch(msg) 
    {
        case WM_CLOSE: 
        {
            g_running = false;
        } break;

        case WM_DESTROY: 
        {
            // @Todo: Handle this as an error - recreate window?
            g_running = false;
        } break;

        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_KEYUP:
        {
            Assert(!"Keyboard input came in through a non-dispatch message!");
        } break;

        case WM_PAINT: 
        {
            PAINTSTRUCT paint;
            HDC hdc = BeginPaint(hwnd, &paint);
            ReleaseDC(hwnd, hdc);
            EndPaint(hwnd, &paint);
        } break;

        case WM_SETCURSOR: 
        {
            if (g_show_cursor) {
                result = DefWindowProcW(hwnd, msg, wparam, lparam);
            } else {
                SetCursor(0);
            }
        } break;

        default: 
        {
            result = DefWindowProcW(hwnd, msg, wparam, lparam);
        } break;
    }

    return result;
}

#if 0
#define fourccRIFF 'RIFF'
#define fourccDATA 'data'
#define fourccFMT 'fmt '
#define fourccWAVE 'WAVE'
#define fourccXWMA 'XWMA'
#define fourccDPDS 'dpds'
#else
#define fourccRIFF 'FFIR'
#define fourccDATA 'atad'
#define fourccFMT ' tmf'
#define fourccWAVE 'EVAW'
#define fourccXWMA 'AMWX'
#define fourccDPDS 'sdpd'
#endif

#if 0
internal b32 
win32_find_chunk(Entire_File *entire_file, DWORD fourcc, u32 *chunk_size, u32 *chunk_data_position)
{
    b32 result = true;

    u32 chunk_type;
    u32 chunk_data_size;
    u32 riff_data_size = 0;
    u32 file_type;
    u32 bytesRead = 0;
    u32 offset = 0;

    u8 *at = (u8 *)entire_file->contents;

    for (;;)
    {
        chunk_type = *(u32 *)at;
        at += sizeof(u32);

        chunk_data_size = *(u32 *)at;
        at += sizeof(u32);

        switch (chunk_type)
        {
            case fourccRIFF:
            {
                riff_data_size = chunk_data_size;
                chunk_data_size = 4;
                file_type = *(u32 *)at;
                at += sizeof(u32);
            } break;

            default:
            {
                at += chunk_data_size;
            } break;
        }

        offset += sizeof(u32) * 2;

        if (chunk_type == fourcc)
        {
            *chunk_size = chunk_data_size;
            *chunk_data_position = offset;
            result = false;
            break;
        }

        offset += chunk_data_size;

        if (bytesRead >= riff_data_size)
        {
            result = false;
            break;
        }
    }

    return result;
}

internal void
win32_read_chunk_data(Entire_File *entire_file, void *buffer, u32 buffer_size, DWORD buffer_offset)
{
    u8 *src = (u8 *)entire_file->contents;
    src += buffer_offset;

    u8 *dst = (u8 *)buffer;
    for (u32 i = 0; i < buffer_size; ++i)
        *dst++ = *src++;
}

internal HRESULT
win32_init_audio() {
    HRESULT hr;

    hr = CoInitializeEx(0, COINIT_MULTITHREADED);
    if (FAILED(hr))
        Assert(!"coinit failed"); // @Todo: error-handling

    IXAudio2 *xaudio = 0;
    if (FAILED(hr = XAudio2Create(&xaudio, 0, XAUDIO2_DEFAULT_PROCESSOR)))
        Assert(!"XAudio init failed"); // @Todo: error-handling

    IXAudio2MasteringVoice *master_voice = 0;
    if (FAILED(hr = xaudio->CreateMasteringVoice(&master_voice)))
        Assert(!"master voice creation failed"); // @Todo: error-handling

    WAVEFORMATEXTENSIBLE wfx = {};
    XAUDIO2_BUFFER buffer = {};

    const char *audio_file_name = "audio/requiem.wav";
    Entire_File entire_file = win32_read_entire_file(audio_file_name);

    u32 chunk_size;
    u32 chunk_position;
    u32 filetype;
    //check the file type, should be fourccWAVE or 'XWMA'
    win32_find_chunk(&entire_file, fourccRIFF, &chunk_size, &chunk_position);
    win32_read_chunk_data(&entire_file, &filetype, sizeof(u32), chunk_position);
    Assert(filetype == fourccWAVE);

    win32_find_chunk(&entire_file, fourccFMT, &chunk_size, &chunk_position);
    win32_read_chunk_data(&entire_file, &wfx, chunk_size, chunk_position);

    //fill out the audio data buffer with the contents of the fourccDATA chunk
    win32_find_chunk(&entire_file, fourccDATA, &chunk_size, &chunk_position);
    u8 *data_buffer = (u8 *)VirtualAlloc(0, chunk_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    win32_read_chunk_data(&entire_file, data_buffer, chunk_size, chunk_position);

    buffer.AudioBytes = chunk_size;  //size of the audio buffer in bytes
    buffer.pAudioData = data_buffer;  //buffer containing audio data
    buffer.Flags = XAUDIO2_END_OF_STREAM; // tell the source voice not to expect any data after this buffer

    IXAudio2SourceVoice* src_voice;
    if (!FAILED(hr = xaudio->CreateSourceVoice(&src_voice, (WAVEFORMATEX *)&wfx))) {
        if (!FAILED(hr = src_voice->SubmitSourceBuffer(&buffer))) {
            if (!FAILED(hr = src_voice->Start(0))) {
                src_voice->SetVolume(0.05f);
            } else {
            }
        } else {
        }
    } else {
    }

    return hr;
}
#endif

internal HWND
win32_create_window(HINSTANCE hinst) 
{
    WNDCLASSEXW wcex = {};
    {
        wcex.cbSize         = sizeof(wcex);
        wcex.style          = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
        wcex.lpfnWndProc    = win32_window_callback;
        wcex.cbClsExtra     = 0;
        wcex.cbWndExtra     = 0;
        wcex.hInstance      = hinst;
        wcex.hIcon          = LoadIcon(NULL, IDI_APPLICATION);
        wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
        wcex.hbrBackground  = (HBRUSH)GetStockObject(BLACK_BRUSH);
        wcex.lpszMenuName   = NULL;
        wcex.lpszClassName  = L"Win32WindowClass";
    }

    if (! RegisterClassExW(&wcex))
    { Assert(! "Win32 couldn't register window class."); }

    HWND hwnd = CreateWindowExW(0, wcex.lpszClassName, L"RTS",
                                WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                                0, 0, hinst, 0);
    return hwnd;
}

internal void
win32_unload_code(Win32_Loaded_Code *loaded)
{
    if (loaded->dll)
    {
        // @Todo: Currently, we never unload libraries, because we may still be pointing to strings that are inside them
        // (despite our best efforts). Should we just make "never unload" be the policy?

        // FreeLibrary(GameCode->GameCodeDLL);
        loaded->dll = 0;
    }

    loaded->is_valid = false;
    zero_array(loaded->functions, loaded->function_count);
}

internal OS_GET_LAST_WRITE_TIME(win32_get_last_write_time_)
{
    u64 result = 0;

    WIN32_FILE_ATTRIBUTE_DATA data;
    if (GetFileAttributesExA(filepath, GetFileExInfoStandard, &data)) {
        result |= data.ftLastWriteTime.dwLowDateTime;
        result |= ((u64)data.ftLastWriteTime.dwHighDateTime << 32);
    }

    return result;
}

internal FILETIME
win32_get_last_write_time(const char *filepath)
{
    FILETIME result = {};

    WIN32_FILE_ATTRIBUTE_DATA data;
    if (GetFileAttributesExA(filepath, GetFileExInfoStandard, &data)) {
        result = data.ftLastWriteTime;
    }

    return result;
}

internal void
win32_build_exe_path_filename(Win32_State *state, char *filename, u32 unique, char *dst, int dst_count)
{
    String a = {
        (umm)(state->one_past_last_exe_filepath_slash - state->exe_filepath),
        (u8 *)state->exe_filepath,
    };

    String b = {};
    {
        b.data = (u8 *)filename;
        b.count = string_length(filename);
    }

    if (unique == 0) {
        str_snprintf(dst, dst_count, "%.*s%.*s", (int)a.count, a.data, (int)b.count, b.data);
    } else {
        str_snprintf(dst, dst_count, "%.*s%u%.*s", (int)a.count, a.data, unique, (int)b.count, b.data);
    }
}

internal void
win32_build_exe_path_filename(Win32_State *state, char *filename, char *dst, int dst_count)
{
    win32_build_exe_path_filename(state, filename, 0, dst, dst_count);
}

internal void
win32_load_code(Win32_State *state, Win32_Loaded_Code *loaded)
{
    char *src_dll_name   = loaded->dll_full_path;
    char *lock_file_name = loaded->lock_full_path;

    char temp_dll_name[WIN32_MAX_PATH_LENGTH];

    WIN32_FILE_ATTRIBUTE_DATA ignored;
    if (!GetFileAttributesExA(lock_file_name, GetFileExInfoStandard, &ignored))
    {
        loaded->dll_last_write_time = win32_get_last_write_time(src_dll_name);

        for (u32 i = 0; i < 128; ++i)
        {
            win32_build_exe_path_filename(state, loaded->transient_dll_name, loaded->temp_dll_name,
                                          temp_dll_name, sizeof(temp_dll_name));

            if (++loaded->temp_dll_name >= 1024) {
                loaded->temp_dll_name = 0;
            }

            if (CopyFileA(src_dll_name, temp_dll_name, FALSE)) {
                break;
            }
        }

        loaded->dll = LoadLibraryA(temp_dll_name);
        if (loaded->dll)
        {
            loaded->is_valid = true;
            for (u32 i = 0; i < loaded->function_count; ++i)
            {
                void *code = (void *)GetProcAddress(loaded->dll, loaded->function_names[i]);
                if (code) {
                    loaded->functions[i] = code;
                }
                else {
                    loaded->is_valid = false;
                }
            }
        }
    }

    if (!loaded->is_valid) {
        win32_unload_code(loaded);
    }
}

internal b32
win32_check_for_code_change(Win32_Loaded_Code *loaded)
{
    FILETIME new_write_time = win32_get_last_write_time(loaded->dll_full_path);
    b32 result = (CompareFileTime(&new_write_time, &loaded->dll_last_write_time) != 0);
    return result;
}

internal void
win32_reload_code(Win32_State *state, Win32_Loaded_Code *loaded)
{
    win32_unload_code(loaded);
    for (u32 attempt = 0; !loaded->is_valid && (attempt < 100); ++attempt) {
        win32_load_code(state, loaded);
        Sleep(100);
    }
}

internal u64
win32_query_os_timer_frequency(void)
{
    LARGE_INTEGER value;
    QueryPerformanceFrequency(&value);
    return value.QuadPart;
}

internal u64
win32_read_os_timer(void)
{
    LARGE_INTEGER value;
    QueryPerformanceCounter(&value);
    return value.QuadPart;
}

internal u64
win32_read_cpu_timer()
{
    u64 result = __rdtsc();
    return result;;
}

internal u64
win32_estimate_cpu_timer_frequency(void)
{
    u64 ms_to_wait = 100;
    u64 os_freq = win32_query_os_timer_frequency();

    u64 cpu_start = win32_read_cpu_timer();
    u64 os_start = win32_read_os_timer();
    u64 os_end = 0;
    u64 os_elapsed = 0;
    u64 os_wait_time = os_freq * ms_to_wait / 1000;
    while (os_elapsed < os_wait_time) {
        os_end = win32_read_os_timer();
        os_elapsed = os_end - os_start;
    }

    u64 cpu_end = win32_read_cpu_timer();
    u64 cpu_elapsed = cpu_end - cpu_start;

    u64 cpu_freq = 0;
    if (os_elapsed) {
        cpu_freq = os_freq * cpu_elapsed / os_elapsed;
    }

    return cpu_freq;
}

OS_GET_SYSTEM_TIME(win32_get_system_time)
{
    Os_Time result = {};
    SYSTEMTIME st = {};
    GetSystemTime(&st);
    result.year = st.wYear;
    result.month = st.wMonth;
    result.dayofweek = st.wDayOfWeek;
    result.day = st.wDay;
    result.hour = st.wHour;
    result.minute = st.wMinute;
    result.second = st.wSecond;
    result.ms = st.wMilliseconds;
    return result;
}

#if BUILD_DEBUG
int wmain(int argc, wchar_t *argv[]) 
{
    HINSTANCE hinst = GetModuleHandleW(0);
#else
int WINAPI
wWinMain(HINSTANCE hinst, HINSTANCE deprecated, PWSTR cmd, int show_cmd)
{
#endif

    os_init();
    {
        os.get_system_time          = win32_get_system_time;
        os.read_cpu_timer           = win32_read_cpu_timer;
        os.get_last_write_time      = win32_get_last_write_time_;
        os.tsc_frequency            = win32_estimate_cpu_timer_frequency();
    }
    thread_init();


    Win32_State *state = &g_win32_state;

    win32_get_exe_filepath(state);

    char game_dll_path[WIN32_MAX_PATH_LENGTH];
    win32_build_exe_path_filename(state, "rts_game.dll", game_dll_path, sizeof(game_dll_path));

    char renderer_dll_path[WIN32_MAX_PATH_LENGTH];
    win32_build_exe_path_filename(state, "win32_opengl.dll", renderer_dll_path, sizeof(renderer_dll_path));

    char code_lock_path[WIN32_MAX_PATH_LENGTH];
    win32_build_exe_path_filename(state, "lock.tmp", code_lock_path, sizeof(code_lock_path));

    // @NOTE: Set the Windows schedular granularity to 1ms so that our Sleep() can be more granular.
    b32 sleep_is_granular = (timeBeginPeriod(1) == TIMERR_NOERROR);
    f32 inv_cpu_timer_freq = 1.0 / os.tsc_frequency;


    HWND hwnd = win32_create_window(hinst);
    if (! hwnd)
    { Assert(! "Win32: Couldn't create window."); }

    state->default_window_handle = hwnd;

#if BUILD_DEBUG
    LPVOID base_address = (LPVOID)TB(2);
#else
    LPVOID base_address = 0;
    win32_toggle_fullscreen(hwnd);
#endif

    HDC renderer_hdc = GetDC(hwnd);

    b32 renderer_was_reloaded = false;
    Win32_Renderer_Function_Table renderer_functions = {};
    Win32_Loaded_Code renderer_code = {};
    renderer_code.transient_dll_name = "renderer_temp.dll";
    renderer_code.dll_full_path = renderer_dll_path;
    renderer_code.lock_full_path = code_lock_path;
    renderer_code.function_count = array_count(win32_renderer_function_table_names);
    renderer_code.functions = (void **)&renderer_functions;
    renderer_code.function_names = win32_renderer_function_table_names;
    win32_load_code(state, &renderer_code);
    if (! renderer_code.is_valid) {
        // @Todo: Error Handling.
        Assert(0);
    }
    umm renderer_memory_size = GB(1);
    void *renderer_memory = VirtualAlloc(0, renderer_memory_size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);

    Arena *renderer_arena = arena_alloc();

    Platform_Renderer *renderer = renderer_functions.load_renderer(renderer_hdc, MB(50), renderer_arena, os);

    win32_load_xinput();
    //HRESULT init_audio_result = win32_init_audio();

    Platform_Game_Memory game_memory = {};
    {
        game_memory.os = os;
        game_memory.arena = arena_alloc();
    }


    u32 monitor_refresh_rate = (u32)GetDeviceCaps(renderer_hdc, VREFRESH);
    f32 desired_dt = 1.0f / (f32)monitor_refresh_rate;

    Input input = {};
    u8 win32_keycode_map[256];
    win32_map_keycode_to_hid_key_code(win32_keycode_map);

    Event_Queue event_queue = {};

    u64 last_cpu_timer = win32_read_cpu_timer();

    Win32_Game_Function_Table game = {};
    Win32_Loaded_Code game_code = {};
    game_code.transient_dll_name = "game_temp.dll";
    game_code.dll_full_path      = game_dll_path;
    game_code.lock_full_path     = code_lock_path;
    game_code.function_count     = array_count(win32_game_function_table_names);
    game_code.functions          = (void **)&game;
    game_code.function_names     = win32_game_function_table_names;

    win32_load_code(state, &game_code);

    ShowWindow(hwnd, SW_SHOW);
    while (g_running) 
    {
        v2u render_dim = {
            1920, 1080,
            //2560, 1440,
        };
        v2u window_dim = win32_get_window_dimension(hwnd);

        game_memory.executable_reloaded = false;

        input.mouse.wheel_delta = 0;

        MSG msg;
        while (PeekMessageA(&msg, hwnd, 0, 0, PM_REMOVE)) 
        {
            switch(msg.message) {
                case WM_QUIT: {
                    g_running = false;
                } break;

                case WM_SYSKEYDOWN:
                case WM_SYSKEYUP:
                case WM_KEYDOWN:
                case WM_KEYUP: {
                    u8 vk_code   = (u8)msg.wParam;
                    b32 was_down = ((msg.lParam & (1 << 30))   != 0);
                    b32 is_down  = ((msg.lParam & (1UL << 31)) == 0);
                    b32 alt      = (msg.lParam & (1 << 29));
                    u8 slot      = win32_keycode_map[vk_code];

                    if (was_down != is_down) {
                        if (event_queue.next_idx < array_count(event_queue.events)) {
                            Event new_event = {};
                            new_event.key = slot;
                            if (is_down) {
                                new_event.flag |= Event_Flag::PRESSED;
                            }
                            else {
                                new_event.flag |= Event_Flag::RELEASED;
                            }
                            event_queue.events[event_queue.next_idx++] = new_event;
                        }

                        if (alt && is_down) {
                            if (vk_code == VK_F4) {
                                g_running = false;
                            }
                            if (vk_code == VK_RETURN && msg.hwnd) {
                                win32_toggle_fullscreen(msg.hwnd);
                            }
                        }

                    }
                } break;

                case WM_MOUSEWHEEL: {
                    s16 z_delta = (GET_WHEEL_DELTA_WPARAM(msg.wParam) / WHEEL_DELTA);
                    input.mouse.wheel_delta = z_delta;
                } break;

                default: {
                    TranslateMessage(&msg);
                    DispatchMessageA(&msg);
                } break;
            }
        }

        {
            Mouse_Input *mouse = &input.mouse;

            POINT mouse_pos;
            GetCursorPos(&mouse_pos);
            ScreenToClient(hwnd, &mouse_pos);
            f32 mouse_x = (f32)mouse_pos.x;
            f32 mouse_y = ((f32)window_dim.h - 1.0f) - (f32)mouse_pos.y;
            input.prev_mouse_p = input.mouse.position;
            input.mouse.position.x = map01(mouse_x, 0.0f, (f32)(window_dim.w - 1)) * (render_dim.w - 1);
            input.mouse.position.y = map01(mouse_y, 0.0f, (f32)(window_dim.h - 1)) * (render_dim.h - 1);

            win32_process_mouse_click(VK_LBUTTON, mouse);
            win32_process_mouse_click(VK_MBUTTON, mouse);
            win32_process_mouse_click(VK_RBUTTON, mouse);
        }

        DWORD result;    
        for (DWORD idx = 0; idx < XUSER_MAX_COUNT; idx++) 
        {
            XINPUT_STATE xinput_state;
            zero_struct(&xinput_state);
            result = xinput_get_state(idx, &xinput_state);
            win32_xinput_handle_deadzone(&xinput_state);
            if (result == ERROR_SUCCESS) {

            } 
            else {
                // Todo: Diagnostic
            }
        }

        u64 cpu_timer = win32_read_cpu_timer();
        f32 dt = (cpu_timer - last_cpu_timer) * inv_cpu_timer_freq;
        last_cpu_timer = cpu_timer;
        if (dt < desired_dt) {
            s32 ms = (s32)((desired_dt - dt) * 1000.0f + 0.5f);
            Sleep(ms);
            dt = desired_dt;
        }
        input.dt        = dt;
        input.actual_dt = dt;
        input.draw_dim  = render_dim;
        input.interacted_ui  = false;

        Render_Commands *render_commands = 0;
        if (renderer_code.is_valid) {
            render_commands = renderer_functions.begin_frame(renderer, window_dim, render_dim);
        }

        if (game.update_and_render) {
            game.update_and_render(&game_memory, &input, &event_queue, render_commands);
        }

        if (input.quit_requested) {
            g_running = false;
        }


#if __DEVELOPER
        b32 needs_to_be_reloaded = win32_check_for_code_change(&game_code);
        if (needs_to_be_reloaded) {
            win32_reload_code(state, &game_code);
        }
#endif


        if (renderer_code.is_valid) {
            if (renderer_was_reloaded) {
                ++render_commands->version;
                renderer_was_reloaded = false;
            }
            renderer_functions.end_frame(renderer, render_commands);
        }

        // We are currently allocating redundant CPU/GPU memory.
        if (win32_check_for_code_change(&renderer_code)) {
            //renderer_functions.cleanup(renderer);
            win32_reload_code(state, &renderer_code);
            renderer_was_reloaded = true;
            renderer = renderer_functions.load_renderer(renderer_hdc, MB(50), renderer_arena, os); 
        }
    }

    return 0;
}
