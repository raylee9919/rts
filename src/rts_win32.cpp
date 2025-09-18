/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Seong Woo Lee $
   $Notice: (C) Copyright %s by Seong Woo Lee. All Rights Reserved. $
   ======================================================================== */


// -----------------------------------
// @Note: [.h]
#include "base/rts_base_inc.h"
#include "os/rts_os.h"
#include "rts_math.h"
#include "rts_asset.h"
#include "rts_input.h"
#include "rts_platform.h"
#include "rts_win32.h"
#include "renderer/rts_renderer.h"

// -----------------------------------
// @Note: [.cpp]
#include "base/rts_base_inc.cpp"
#include "os/rts_os.cpp"
#include "rts_math.cpp"


// -----------------------------------
// @Note: Windows Additional Libs
#include <dwmapi.h>
#include <psapi.h>

#pragma comment(lib, "dwmapi")


// -----------------------------------
// @Note: Globals
global Win32_State          win32;
global b32                  g_running = true;
global b32                  g_show_cursor = true;
global WINDOWPLACEMENT      g_window_placement = {sizeof(g_window_placement)};

#include "rts_win32_renderer.h"


// ----------------------------------------------------
// @Note: Input.
// @Todo: bad :(
internal void
win32_map_keycode_to_hid_key_code(u8 *map) 
{
    map[VK_BACK]      = KEY_BACKSPACE;
    map[VK_TAB]       = KEY_TAB;
    map[VK_RETURN]    = KEY_ENTER;
    map[VK_SHIFT]     = KEY_LEFTSHIFT;
    map[VK_LSHIFT]    = KEY_LEFTSHIFT;
    map[VK_CONTROL]   = KEY_LEFTCTRL;
    map[VK_LCONTROL]  = KEY_LEFTCTRL;
    map[VK_LMENU]     = KEY_LEFTALT;
    map[VK_ESCAPE]    = KEY_ESC;
    map[VK_SPACE]     = KEY_SPACE;
    map[VK_OEM_3]     = KEY_HASHTILDE;
    map[VK_LEFT]      = KEY_LEFT;
    map[VK_RIGHT]     = KEY_RIGHT;
    map[VK_UP]        = KEY_UP;
    map[VK_DOWN]      = KEY_DOWN;
    map[VK_OEM_PLUS]  = KEY_EQUAL;
    map[VK_OEM_MINUS] = KEY_MINUS;
    for (char c = 'A'; c <= 'Z'; ++c)
    { map[c] = KEY_A + (c - 'A'); }
    for (char c = 0x30; c <= 0x39; ++c) // 0~9
    { map[c] = KEY_0 + (c - 0x30); }
    for (char c = VK_F1; c <= VK_F12; ++c)
    { map[c] = KEY_F1 + (c - VK_F1); }
}

internal void
win32_process_keyboard(Game_Key *game_key, b32 is_down) 
{
    if (is_down) { game_key->is_down = true; }
    else         { game_key->is_down = false; }
}

internal void
win32_process_mouse_click(s32 vk, Mouse_Input *mouse) 
{
    b32 is_down = GetKeyState(vk) & (1 << 15);
    u32 E = 0;

    switch(vk) 
    {
        case VK_LBUTTON: { E = Mouse_Left;   } break;
        case VK_MBUTTON: { E = Mouse_Middle; } break;
        case VK_RBUTTON: { E = Mouse_Right;  } break;
        INVALID_DEFAULT_CASE;
    }

    if (is_down) 
    {
        if (! mouse->is_down[E]) 
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

// ----------------------------------------------------
// @Note: Window
internal LRESULT 
win32_window_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) 
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
    { Assert(! "Win32 couldn't register window class."); }

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

// ----------------------------------------------------
// @Note: Code Reloading
internal u64
win32_get_last_modified(Utf8 file_path)
{
    Os_File_Attributes attr = os.attributes_from_file_path(file_path);
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

    Os_File_Attributes attr = os.attributes_from_file_path(dll_path);

    if (attr.size > 0)
    {
        // load the temporary dll so we could write to the real dll and check the modified time of it.
        loaded->temp_dll_path_prefix = (loaded->temp_dll_path_prefix + 1) % 2;
        temp_dll_path = utf8f(scratch.arena, "%S/%S_%d.dll", win32.binary_path, loaded->temp_dll_name, loaded->temp_dll_path_prefix);
        os.file_copy(temp_dll_path, dll_path);

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


// ----------------------------------------------------
// @Note: Entry
#if BUILD_DEBUG
int wmain(int argc, wchar_t *argv[]) 
{
    HINSTANCE hinst = GetModuleHandleW(0);
#else
int WINAPI
wWinMain(HINSTANCE hinst, HINSTANCE deprecated, PWSTR cmd, int show_cmd)
{
#endif
    // ----------------------------------------
    // @Note: init core.
    os_init();
    thread_init();

    // ----------------------------------------
    // @Note: init platform.
    Platform platform = {};
    Utf8 binary_path = {};
    {
        {
            platform.arena = arena_alloc();
            platform.os = os;
        }

        {
            Temporary_Arena scratch = scratch_begin();

            binary_path = os.string_from_system_path_kind(scratch.arena, OS_SYSTEM_PATH_KIND_BINARY);
            Utf8 local_data_path = utf8f(scratch.arena, "%S/data", binary_path);
            Utf8 binary_parent_path = utf8_path_chop_last_slash(binary_path);
            Utf8 parent_data_path = utf8f(scratch.arena, "%S/data", binary_parent_path);

            Os_File_Attributes local_data_attr  = os.attributes_from_file_path(local_data_path);
            Os_File_Attributes parent_data_attr = os.attributes_from_file_path(parent_data_path);

            if (local_data_attr.flags == OS_FILE_FLAG_DIRECTORY)
            { platform.data_path = utf8_copy(platform.arena, local_data_path); }
            else if (parent_data_attr.flags == OS_FILE_FLAG_DIRECTORY)
            { platform.data_path = utf8_copy(platform.arena, parent_data_path); }

            scratch_end(scratch);
        }
    }


    // ----------------------------------------
    // @Note: create window.

    // this must be place before creating window.
    if (FAILED(SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2)))
    { os.abort(); }

    HWND hwnd = win32_window_create(hinst);
    if (! hwnd) { Assert(! "Win32: Couldn't create window."); }
    win32_window_update_dark_mode(hwnd);




    // ----------------------------------------
    // @Note: init win32 state.
    {
        win32.arena = arena_alloc();

        win32.main_hwnd = hwnd;

        win32.binary_path        = binary_path;
        win32.game_dll_path      = utf8f(win32.arena, "%S/rts_game.dll", binary_path);
        win32.renderer_dll_path  = utf8f(win32.arena, "%S/rts_renderer_opengl.dll", binary_path);
        win32.lock_path          = utf8f(win32.arena, "%S/lock.tmp", binary_path);
    }


#if !BUILD_DEBUG
    win32_toggle_fullscreen(hwnd);
#endif

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
    { Assert(! "Couldn't load the renderer code."); }

    Arena *renderer_arena = arena_alloc();
    Platform_Renderer *renderer = renderer_functions.load_renderer(renderer_hdc, MB(50), renderer_arena, os);




    u32 monitor_refresh_rate = (u32)GetDeviceCaps(renderer_hdc, VREFRESH);
    f32 desired_dt = (1.0f / (f32)monitor_refresh_rate);

    Input input = {};
    u8 win32_keycode_map[256];
    win32_map_keycode_to_hid_key_code(win32_keycode_map);

    Event_Queue event_queue = {};



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
    u64 old_counter = os.perf_counter();
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

        // @Note: draw resolution.
        v2u render_dim = {
            1920, 1080,
            //2560, 1440,
        };
        v2u window_dim = win32_client_size(hwnd);

        input.mouse.wheel_delta = 0;

        for (MSG msg; PeekMessage(&msg, hwnd, 0, 0, PM_REMOVE);)
        {
            switch(msg.message) 
            {
                case WM_QUIT: {
                    g_running = false;
                } break;

                case WM_SYSKEYDOWN:
                case WM_SYSKEYUP:
                case WM_KEYDOWN:
                case WM_KEYUP: {
                    if (! win32_window_focused(hwnd))
                    { break;}

                    u8 vk_code   = (u8)msg.wParam;
                    b32 was_down = ((msg.lParam & (1 << 30))   != 0);
                    b32 is_down  = ((msg.lParam & (1UL << 31)) == 0);
                    b32 alt      = (msg.lParam & (1 << 29));
                    u8 slot      = win32_keycode_map[vk_code];

                    if (was_down != is_down) 
                    {
                        if (event_queue.next_idx < array_count(event_queue.events)) 
                        {
                            Event new_event = {};
                            new_event.key = slot;
                            if (is_down) 
                            {
                                new_event.flag |= Event_Flag::PRESSED;
                            }
                            else 
                            {
                                new_event.flag |= Event_Flag::RELEASED;
                            }
                            event_queue.events[event_queue.next_idx++] = new_event;
                        }

                        if (alt && is_down) 
                        {
                            if (vk_code == VK_F4) 
                            {
                                g_running = false;
                            }
                            if (vk_code == VK_RETURN && msg.hwnd) 
                            {
                                win32_toggle_fullscreen(msg.hwnd);
                            }
                        }

                    }
                } break;

                case WM_MOUSEWHEEL: {
                    if (! win32_window_focused(hwnd))
                    { break;}

                    s16 z_delta = (GET_WHEEL_DELTA_WPARAM(msg.wParam) / WHEEL_DELTA);
                    input.mouse.wheel_delta = z_delta;
                } break;

                default: {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                } break;
            }
        }

        if (win32_window_focused(hwnd))
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


        // ----------------------------------------
        // @Note: get dt.
        u64 new_counter = os.perf_counter();
        f32 dt = (new_counter - old_counter) * os.perf_counter_freq_inv;
        old_counter = new_counter;
        if (dt < desired_dt) 
        {
            s32 ms = (s32)((desired_dt - dt) * 1000.0f + 0.5f);
            if (os.sleep_is_granular)
            {
                // @Todo: what?
            }
            Sleep(ms);
            dt = desired_dt;
        }

        {
            input.dt        = dt;
            input.actual_dt = dt;
            input.draw_dim  = render_dim;
            input.interacted_ui  = false;
        }

        Render_Commands *render_commands = 0;
        if (renderer_code.is_valid) 
        { render_commands = renderer_functions.begin_frame(renderer, window_dim, render_dim); }

        if (game.update_and_render) 
        { game.update_and_render(&platform, &input, &event_queue, render_commands); }

        if (input.quit_requested) 
        { g_running = false; }


        if (win32_code_modified(&game_code)) 
        {
            win32_code_reload(&game_code); 
            game_code.last_modified = win32_get_last_modified(game_code.dll_path);
        }


        if (renderer_code.is_valid) 
        {
            if (renderer_was_reloaded) 
            {
                ++render_commands->version;
                renderer_was_reloaded = false;
            }
            renderer_functions.end_frame(renderer, render_commands);
        }

        // @Fix: We are currently allocating redundant CPU/GPU memory.
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
