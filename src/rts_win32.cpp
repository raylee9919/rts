/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Seong Woo Lee $
   $Notice: (C) Copyright %s by Seong Woo Lee. All Rights Reserved. $
   ======================================================================== */

// # Todo: 1. Should we support scalable dpi?
//         2. Verfiy the OS version and load the appropriate libraries.
//


// # Note: [.h]
//
#include "base/rts_base_inc.h"
#include "os/rts_os.h"
#include "rts_font.h"
#include "rts_asset.h"
#include "rts_platform.h"
#include "rts_win32.h"
#include "renderer/rts_renderer.h"
#include "rts_win32_renderer.h"

// # Note: Globals
//
global Renderer *g_renderer;

// # Note: [.cpp]
//
#include "base/rts_base_inc.cpp"
#include "os/rts_os.cpp"


// # Note: Windows Additional Libs
//
#include <windowsx.h>
#include <dwmapi.h>
#include <psapi.h>

#pragma comment(lib, "dwmapi")

// # Note: Executables (but not DLLs) exporting this symbol with this value will be
//         automatically directed to the high-performance GPU on Nvidia Optimus systems
//         with up-to-date drivers
//
__declspec(dllexport) DWORD NvOptimusEnablement = 1;

// # Note: Executables (but not DLLs) exporting this symbol with this value will be
//         automatically directed to the high-performance GPU on AMD PowerXpress systems
//         with up-to-date drivers
//
__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;

// # Note: Globals
//
global Win32_State          win32;
global b32                  g_running = true;
global b32                  g_show_cursor = true;
global WINDOWPLACEMENT      g_window_placement = {sizeof(g_window_placement)};

// # Note: Window
//
internal LRESULT 
win32_window_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) 
{
    LRESULT result = 0;

    Os_Event *event = NULL;

    Os_Event_Type type  = OS_EVENT_NULL;
    Os_Key key          = OS_KEY_NULL;
    v2 axis             = v2{0,0};
    b32 is_release      = 0;

    switch(msg) 
    {
        case WM_CLOSE: {
            g_running = false;
        } break;

        case WM_DESTROY: {
            // @Todo: Handle this as an error - recreate window?
            g_running = false;
        } break;

        // --------------------------------------------
        // @Note: Keyboard

        // @Note: Keyboard Events
        //
        case WM_SYSKEYDOWN: 
        case WM_SYSKEYUP: 
        {
            DefWindowProcW(hwnd, msg, wparam, lparam);
        }
        case WM_KEYDOWN: 
        case WM_KEYUP:
        {
            b32 was_down = !!(lparam & (1 << 30));
            b32 is_down =   !(lparam & (1 << 31));

            type = is_down ? OS_EVENT_PRESS : OS_EVENT_RELEASE;

            if (wparam < array_count(os->key_table))
            {
                key = os->key_table[wparam];
            }

            event = os_event_alloc();
            {
                event->type = type;
                event->key  = key;
            }
            dll_push_back(os->event_sentinel, event);
        } break;

        // @Note: Text Input
        //
        case WM_CHAR: 
        case WM_SYSCHAR:
        {
            u32 c = wparam;
            if (c == '\r') { c = '\n'; }

            if ((c >= 32 && c != 127/*DEL*/) || c == '\t' || c == '\n')
            {
                event = os_event_alloc();
                {
                    event->type      = OS_EVENT_TEXT;
                    event->character = c;
                }
                dll_push_back(os->event_sentinel, event);
            }
        } break;


        // --------------------------------------------
        // @Note: Mouse

        // @Note: Mouse Move
        //
        case WM_MOUSEMOVE: 
        {
            int x = GET_X_LPARAM(lparam);
            int y = GET_Y_LPARAM(lparam);

            os->mouse_position_last = v2{(f32)x, (f32)y};

            type = OS_EVENT_MOUSE_MOVE;

            event = os_event_alloc();
            {
                event->type       = type;
                event->position.x = (f32)x;
                event->position.y = (f32)y;
            }
            dll_push_back(os->event_sentinel, event);
        } break;


        // @Note: Mouse Scroll
        //
        case WM_MOUSEHWHEEL: 
        {
            axis = v2{1,0};
            goto winproc_mouse_scroll;
        } break;
        case WM_MOUSEWHEEL: 
        {
            axis = v2{0,1};
winproc_mouse_scroll:;
            type = OS_EVENT_MOUSE_SCROLL;
            f32 delta = (f32)GET_WHEEL_DELTA_WPARAM(wparam);

            POINT p;
            {
                p.x = GET_X_LPARAM(lparam);
                p.y = GET_Y_LPARAM(lparam);
                ScreenToClient(hwnd, &p);
            }

            event = os_event_alloc();
            {
                event->type       = type;
                event->delta      = delta*axis;
                event->position.x = (f32)p.x;
                event->position.y = (f32)p.y;
            }
            dll_push_back(os->event_sentinel, event);
        } break;



        // @Note: Mouse Click
        //
        case WM_LBUTTONUP: { ReleaseCapture(); is_release = 1; };
        case WM_LBUTTONDOWN: {
            key = OS_KEY_MOUSE_LEFT;
            goto winproc_mouse;
       }

        case WM_RBUTTONUP: { ReleaseCapture(); is_release = 1; };
        case WM_RBUTTONDOWN: {
            key = OS_KEY_MOUSE_RIGHT;
            goto winproc_mouse;
        }

        case WM_MBUTTONUP: { ReleaseCapture(); is_release = 1; };
        case WM_MBUTTONDOWN: {
            key = OS_KEY_MOUSE_MIDDLE;
winproc_mouse:;
              if (! is_release)
              {
                  SetCapture(hwnd);
              }

              int x = GET_X_LPARAM(lparam);
              int y = GET_Y_LPARAM(lparam);

              event = os_event_alloc();
              {
                  event->type       = is_release ? OS_EVENT_RELEASE : OS_EVENT_PRESS;
                  event->key        = key;
                  event->position.x = (f32)x;
                  event->position.y = (f32)y;
              }
              dll_push_back(os->event_sentinel, event);
        } break;

        case WM_PAINT: {
            PAINTSTRUCT paint;
            HDC hdc = BeginPaint(hwnd, &paint);
            ReleaseDC(hwnd, hdc);
            EndPaint(hwnd, &paint);
        } break;

        case WM_SETCURSOR: {
            if (g_show_cursor) {
                result = DefWindowProcW(hwnd, msg, wparam, lparam);
            } else {
                SetCursor(0);
            }
        } break;

        default: {
            result = DefWindowProcW(hwnd, msg, wparam, lparam);
        } break;
    }

    if (event != NULL)
    {
        event->modifiers = os->get_modifiers();
    }

    return result;
}

internal HWND
win32_window_create(HINSTANCE hinst) 
{
    WNDCLASSEXW wcex = {};
    {
        wcex.cbSize         = sizeof(wcex);
        wcex.style          = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
        wcex.lpfnWndProc    = win32_window_proc;
        wcex.cbClsExtra     = 0;
        wcex.cbWndExtra     = 0;
        wcex.hInstance      = hinst;
        wcex.hIcon          = LoadIcon(hinst, L"Icon");
        wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
        wcex.hbrBackground  = (HBRUSH)GetStockObject(BLACK_BRUSH);
        wcex.lpszMenuName   = NULL;
        wcex.lpszClassName  = L"Win32WindowClass";
    }

    if (! RegisterClassExW(&wcex))
    { assert(! "Win32 couldn't register window class."); }

    HWND hwnd = CreateWindowExW(0, wcex.lpszClassName, L"RTS",
                                WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                                0, 0, hinst, 0);
    return hwnd;
}

internal b32
win32_window_focused(HWND hwnd)
{
    return hwnd == GetFocus();
}

internal void
win32_window_update_dark_mode(HWND hwnd)
{
    // @Note: not really necessary... but why not?
    HMODULE uxtheme = LoadLibraryExW(L"uxtheme.dll", 0, LOAD_LIBRARY_SEARCH_SYSTEM32);
    BOOL(WINAPI *func)() = 0;
    if (uxtheme)
    {
        func = (BOOL(WINAPI *)())GetProcAddress(uxtheme, MAKEINTRESOURCEA(132));
    }

    if (func)
    {
        BOOL high_contrast = false;
        HIGHCONTRAST hc = {sizeof(hc)};
        if (SystemParametersInfoW(SPI_GETHIGHCONTRAST, sizeof(hc), &hc, 0))
        {
            high_contrast = (HCF_HIGHCONTRASTON & hc.dwFlags) != 0;
        }

        BOOL use_dark_mode = (func() && !high_contrast);

        if (use_dark_mode)
        {
            DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &use_dark_mode, sizeof(BOOL));
        }
    }
}

internal void 
win32_toggle_fullscreen(HWND window)
{
    // @Note: Thank you, Raymond Chen.
    DWORD style = GetWindowLong(window, GWL_STYLE);
    if (style & WS_OVERLAPPEDWINDOW) 
    {
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
    }
    else 
    {
        SetWindowLong(window, GWL_STYLE, style | WS_OVERLAPPEDWINDOW);
        SetWindowPlacement(window, &g_window_placement);
        SetWindowPos(window, 0, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
                     SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    }
}

internal v2u 
win32_client_size(HWND hwnd) 
{
    v2u result = {};
    RECT rect;
    GetClientRect(hwnd, &rect);
    result.w = (u32)(rect.right - rect.left);
    result.h = (u32)(rect.bottom - rect.top);
    return result;
}

// -----------------------------------
// @Note: Code Reloading
internal u64
win32_get_last_modified(Utf8 file_path)
{
    Os_File_Attributes attr = os->attributes_from_file_path(file_path);
    u64 result = attr.last_modified;
    return result;
}

internal void
win32_code_unload(Win32_Code *loaded)
{
    if (loaded->dll)
    {
        // @Todo: Currently, we never unload libraries, because we may still be pointing to strings that are inside them
        //        (despite our best efforts). Should we just make "never unload" be the policy?
        // FreeLibrary(loaded->dll);
        loaded->dll = 0;
    }

    loaded->is_valid = false;
    zero_array(loaded->functions, loaded->function_count);
}

internal void
win32_code_load(Win32_Code *loaded)
{
    Temporary_Arena scratch = scratch_begin();

    Utf8 dll_path       = loaded->dll_path;
    Utf8 temp_dll_path  = loaded->temp_dll_path;
    Utf8 lock_path      = loaded->lock_path;

    Os_File_Attributes attr = os->attributes_from_file_path(dll_path);

    if (attr.size > 0)
    {
        // load the temporary dll so we could write to the real dll and check the modified time of it.
        loaded->temp_dll_path_prefix = (loaded->temp_dll_path_prefix + 1) % 2;
        temp_dll_path = utf8f(scratch.arena, "%S/%S_%d.dll", win32.binary_path, loaded->temp_dll_name, loaded->temp_dll_path_prefix);
        os->file_copy(temp_dll_path, dll_path);

        Utf16 temp_dll_path16 = to_utf16(scratch.arena, temp_dll_path);
        loaded->dll = LoadLibraryW((WCHAR *)temp_dll_path16.str);

        // if loaded properly, get proc addresses.
        if (loaded->dll)
        {
            loaded->is_valid = true;

            for (u32 i = 0; i < loaded->function_count; ++i)
            {
                void *code = (void *)GetProcAddress(loaded->dll, loaded->function_names[i]);
                if (code) 
                {
                    loaded->functions[i] = code;
                }
                else 
                {
                    loaded->is_valid = false;
                }
            }
        }
    }

    // if libary nor proc isn't loaded, unload the code.
    if (! loaded->is_valid) 
    {
        win32_code_unload(loaded);
    }

    scratch_end(scratch);
}

internal void
win32_code_reload(Win32_Code *loaded)
{
    win32_code_unload(loaded);
    win32_code_load(loaded);
}

internal b32
win32_code_modified(Win32_Code *loaded)
{
    u64 last_modified = win32_get_last_modified(loaded->dll_path);
    b32 result = (last_modified != loaded->last_modified);
    return result;
}

// # Note: Entry
//
#if 0
int wmain(int argc, wchar_t *argv[]) 
{
    HINSTANCE hinst = GetModuleHandleW(0);
#else
int WINAPI
wWinMain(HINSTANCE hinst, HINSTANCE deprecated, PWSTR cmd, int show_cmd)
{
#endif

    // # Note: init core.
    //
    os_init();
    thread_init();

    // # Note: init platform.
    //
    Platform platform = {};
    Utf8 binary_path = {};
    {
        {
            platform.arena = arena_alloc();
            platform.os = os;
        }

        {
            Temporary_Arena scratch = scratch_begin();

            binary_path = os->string_from_system_path_kind(scratch.arena, OS_SYSTEM_PATH_KIND_BINARY);
            Utf8 local_data_path = utf8f(scratch.arena, "%S/data", binary_path);
            Utf8 binary_parent_path = utf8_path_chop_last_slash(binary_path);
            Utf8 parent_data_path = utf8f(scratch.arena, "%S/data", binary_parent_path);

            Os_File_Attributes local_data_attr  = os->attributes_from_file_path(local_data_path);
            Os_File_Attributes parent_data_attr = os->attributes_from_file_path(parent_data_path);

            if (local_data_attr.flags == OS_FILE_FLAG_DIRECTORY)
            { platform.data_path = utf8_copy(platform.arena, local_data_path); }
            else if (parent_data_attr.flags == OS_FILE_FLAG_DIRECTORY)
            { platform.data_path = utf8_copy(platform.arena, parent_data_path); }

            scratch_end(scratch);
        }
    }


    // # Note: init gfx.
    //
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    HWND hwnd = win32_window_create(hinst);
    if (! hwnd) 
    {
        assert(! "Win32: Couldn't create window."); 
    }
    win32_window_update_dark_mode(hwnd);




    // # Note: init win32 state.
    // 
    {
        win32.arena = arena_alloc();

        win32.main_hwnd = hwnd;

        win32.binary_path        = binary_path;
        win32.game_dll_path      = utf8f(win32.arena, "%S/rts_game.dll", binary_path);
        win32.renderer_dll_path  = utf8f(win32.arena, "%S/rts_renderer_opengl.dll", binary_path);
        win32.lock_path          = utf8f(win32.arena, "%S/lock.tmp", binary_path);
    }


    // # Note: toggle fullscreen if needed.
    // 
#if !BUILD_DEBUG
    win32_toggle_fullscreen(hwnd);
#endif

    // # Note: alloc/init renderer.
    // 
    {
        Arena *arena = arena_alloc();
        g_renderer = push_struct(arena, Renderer);
        g_renderer->arena = arena;
        platform.renderer = g_renderer;
    }


    HDC renderer_hdc = GetDC(hwnd);
    b32 renderer_was_reloaded = false;
    Win32_Renderer_Function_Table renderer_functions = {};
    Win32_Code renderer_code = {};
    {
        renderer_code.temp_dll_name  = utf8lit("renderer_temp");
        renderer_code.temp_dll_path  = utf8f(win32.arena, "%S/%S.dll", win32.binary_path, renderer_code.temp_dll_name);
        renderer_code.dll_path       = win32.renderer_dll_path;
        renderer_code.lock_path      = win32.lock_path;
        renderer_code.function_count = array_count(win32_renderer_function_table_names);
        renderer_code.functions      = (void **)&renderer_functions;
        renderer_code.function_names = win32_renderer_function_table_names;
        renderer_code.last_modified  = win32_get_last_modified(renderer_code.dll_path);
    }
    win32_code_load(&renderer_code);
    if (! renderer_code.is_valid) 
    { assert(! "Couldn't load the renderer code."); }

    Arena *renderer_arena = arena_alloc();
    Platform_Renderer *renderer = renderer_functions.load_renderer(renderer_hdc, MB(50), renderer_arena, os);



    u32 monitor_refresh_rate = (u32)GetDeviceCaps(renderer_hdc, VREFRESH);
    f32 desired_dt = (1.0f / (f32)monitor_refresh_rate);

    Win32_Game_Function_Table game = {};
    Win32_Code game_code = {};
    {
        game_code.temp_dll_name      = utf8lit("game_temp");
        game_code.temp_dll_path      = utf8f(win32.arena, "%S/%S.dll", win32.binary_path, game_code.temp_dll_name);
        game_code.dll_path           = win32.game_dll_path;
        game_code.lock_path          = win32.lock_path;
        game_code.function_count     = array_count(win32_game_function_table_names);
        game_code.functions          = (void **)&game;
        game_code.function_names     = win32_game_function_table_names;
        game_code.last_modified      = win32_get_last_modified(game_code.dll_path);
    }
    win32_code_load(&game_code);


    // ----------------------------------------
    // @Note: Main Loop
    u64 old_counter = os->perf_counter();
    while (g_running) 
    {
        {
            // @Temporary: just learning win32 calls of gathering memory status.
            MEMORYSTATUSEX ms;
            ms.dwLength = sizeof(ms);
            GlobalMemoryStatusEx(&ms);


            HANDLE proc = GetCurrentProcess();
            PROCESS_MEMORY_COUNTERS pmc;
            GetProcessMemoryInfo(proc,  &pmc, sizeof(pmc));
            CloseHandle(proc);
        }

        // # Note: draw resolution.
        v2u render_dim = {
            1920, 1080,
            //2560, 1440,
        };
        v2u window_dim = win32_client_size(hwnd);

        os_event_list_clear();
        os->event_poll();


        // # Note: Process Message
        //
        for (MSG msg; PeekMessage(&msg, hwnd, 0, 0, PM_REMOVE);)
        {
            switch(msg.message) 
            {
                case WM_QUIT: {
                    g_running = false;
                } break;

                default: {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                } break;
            }
        }


        // # Note: Fullscreen
        //
        for (Os_Event *ev = os->event_sentinel->next, *next = NULL;
             ev != os->event_sentinel;
             ev = next)
        {
            next = ev->next;
            if (ev->type == OS_EVENT_PRESS && (ev->modifiers & OS_MODIFIER_ALT) && ev->key == OS_KEY_ENTER)
            {
                win32_toggle_fullscreen(hwnd);
                os_event_consume(ev);
            }
        }


        // # Note: get dt.
        //
        u64 new_counter = os->perf_counter();
        f32 dt = (new_counter - old_counter) * os->perf_counter_freq_inv;
        old_counter = new_counter;
        if (dt < desired_dt) 
        {
            s32 ms = (s32)((desired_dt - dt) * 1000.0f + 0.5f);
            if (os->sleep_is_granular)
            {
                // # Todo: what?
            }
            Sleep(ms);
            dt = desired_dt;
        }

        {
            platform.dt = dt;
            platform.draw_width  = render_dim.x;
            platform.draw_height = render_dim.y;
        }

        Render_Commands *render_commands = NULL;

        // # Note: Render begin.
        //
        if (renderer_code.is_valid) 
        {
            render_commands = renderer_functions.begin_frame(renderer, window_dim, render_dim); 
        }

        // # Note: Game update and render.
        //
        if (game.update_and_render) 
        {
            game.update_and_render(&platform, render_commands); 
        }

        // # Note: Game code hot reloading.
        //
        if (win32_code_modified(&game_code)) 
        {
            win32_code_reload(&game_code); 
            game_code.last_modified = win32_get_last_modified(game_code.dll_path);
        }

        // # Note: Render end.
        //
        if (renderer_code.is_valid) 
        {
            if (renderer_was_reloaded) 
            {
                renderer_was_reloaded = false;
            }
            renderer_functions.end_frame(renderer, g_renderer, render_commands);
        }

        // # Fix: We are currently allocating redundant CPU/GPU memory.
        //if (win32_code_modified(&renderer_code)) 
        //{
        //    //renderer_functions.cleanup(renderer);
        //    win32_code_reload(&win32, &renderer_code);
        //    renderer_was_reloaded = true;
        //    renderer = renderer_functions.load_renderer(renderer_hdc, MB(50), renderer_arena, os); 
        //}
    }

    return 0;
}
