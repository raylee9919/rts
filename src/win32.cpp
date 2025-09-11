/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Sung Woo Lee $
   $Notice: (C) Copyright %s by Sung Woo Lee. All Rights Reserved. $
   ======================================================================== */




#include <windows.h>
#include <Xinput.h>
#include <xaudio2.h>

#include <stdio.h>

#include "types.h"
#include "shared.h"
#include "intrinsics.h"
#include "memory.cpp"
#include "math.h"
#include "asset.h"
#include "model.h"
#include "input.h"
#include "platform.h"
#include "win32.h"

#include "win32_xinput.cpp"
#include "win32_keycode.cpp"

Platform_Api os;

global Win32_State          g_win32_state;
global b32                  g_running = true;
global b32                  g_show_cursor;
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

internal PLATFORM_GET_WORKING_DIRECTORY(win32_get_working_directory)
{
    Assert(GetCurrentDirectory(size, buffer) != 0);
}

internal void 
win32_toggle_fullscreen(HWND window)
{
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
win32_get_filetime(LPCSTR filename) 
{
    FILETIME result = {};
    WIN32_FIND_DATA find_data;
    FindFirstFileA(filename, &find_data);
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
win32_get_file_handle(Platform_File_Handle *handle)
{
    return *(HANDLE *)&handle->platform;
}

internal PLATFORM_OPEN_FILE(win32_open_file)
{
    Platform_File_Handle result = {};
    static_assert(sizeof(HANDLE) <= sizeof(result.platform));
    
    DWORD permissions = 0;
    DWORD creation = 0;

    if (flags & Open_File_Read) {
        permissions |= GENERIC_READ;
        creation = OPEN_EXISTING;
    }
    if (flags & Open_File_Write) {
        permissions |= GENERIC_WRITE;
        creation = OPEN_ALWAYS;
    }
    
    HANDLE handle = CreateFileA(filepath, permissions, FILE_SHARE_READ, 0, creation, 0, 0);
    Assert(handle != INVALID_HANDLE_VALUE);

    *(HANDLE *)&result.platform = handle;
    
    return result;
}

internal PLATFORM_CLOSE_FILE(win32_close_file)
{
    HANDLE file = win32_get_file_handle(handle);

    if (file) {
        CloseHandle(file);
    }
}

internal PLATFORM_GET_FILE_SIZE(win32_get_file_size)
{
    u32 result = 0;
    HANDLE win32_handle = win32_get_file_handle(handle);
    if (win32_handle) {
        LARGE_INTEGER win32_handlesize;
        GetFileSizeEx(win32_handle, &win32_handlesize);
        result = win32_handlesize.QuadPart;
    }
    return result;
}

internal PLATFORM_READ_FROM_FILE(win32_read_from_file) 
{
    HANDLE win32_handle = win32_get_file_handle(handle);
    DWORD bytes_read;
    if (ReadFile(win32_handle, dst, size, &bytes_read, 0) && (size == bytes_read)) {

    } else {
        Assert(0);
    }
}

internal PLATFORM_READ_ENTIRE_FILE(win32_read_entire_file)
{
    Buffer result = {};
    Platform_File_Handle handle = win32_open_file(filepath, Open_File_Read);
    umm filesize = win32_get_file_size(&handle);
    result.count = filesize;
    result.data = (u8 *)VirtualAlloc(0, filesize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    DWORD bytes_read;
    HANDLE win32_handle = win32_get_file_handle(&handle); 
    ReadFile(win32_handle, result.data, result.count, &bytes_read, 0);
    CloseHandle(win32_handle);
    return result;
}

internal PLATFORM_FREE_MEMORY(win32_free_memory)
{
    if (memory) {
        VirtualFree(memory, 0, MEM_RELEASE);
    }
}

internal PLATFORM_COPY_FILE(win32_copy_file)
{
    CopyFile(src, dst, FALSE);
}

internal void
win32_begin_recording_input(Win32_State *win32_state) 
{
    win32_state->is_recording = 1;
    const char *filename = "inputNstate.rec";
    win32_state->record_file = CreateFileA(filename, GENERIC_WRITE,
                                           0, 0, CREATE_ALWAYS, 0, 0);
    DWORD bytes_written;
    DWORD bytes_to_write = (DWORD)win32_state->game_memory_total_size;
    Assert(bytes_to_write == win32_state->game_memory_total_size);
    WriteFile(win32_state->record_file, win32_state->game_memory, 
              (DWORD)win32_state->game_memory_total_size, &bytes_written, 0);
}

internal void
win32_record_input(Win32_State *win32_state, Input *input) 
{
    DWORD bytes_written;
    WriteFile(win32_state->record_file, input, sizeof(*input),
              &bytes_written, 0);
}

internal void
win32_end_input_recording(Win32_State *win32_state) 
{
    CloseHandle(win32_state->record_file);
    win32_state->is_recording = 0;
}

internal void
win32_begin_input_playback(Win32_State *win32_state) 
{
    win32_state->is_playing = 1;

    const char *filename = "inputNstate.rec";
    win32_state->record_file = CreateFileA(filename, GENERIC_READ,
                                           FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);

    DWORD bytes_read;
    ReadFile(win32_state->record_file, win32_state->game_memory,
             (DWORD)win32_state->game_memory_total_size, &bytes_read, 0);
}

internal void
win32_end_input_playback(Win32_State *win32_state) 
{
    CloseHandle(win32_state->record_file);
    win32_state->is_playing = 0;
}

internal void
win32_playback_input(Win32_State *win32_state, Input *input) 
{
    DWORD bytes_read;
    if (ReadFile(win32_state->record_file, input,
                 sizeof(*input), &bytes_read, 0)) 
    {
        if (bytes_read == 0) 
        {
            win32_end_input_playback(win32_state);
            win32_begin_input_playback(win32_state);
        }
    }
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
            // @TODO: Handle this as an error - recreate window?
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
                result = DefWindowProcA(hwnd, msg, wparam, lparam);
            } else {
                SetCursor(0);
            }
        } break;

        default: 
        {
            result = DefWindowProcA(hwnd, msg, wparam, lparam);
        } break;
    }

    return result;
}

///////////////////////////////////////////////////////////////////////////////
//
// Multi-Threading
// 

struct Platform_Work_QueueEntry 
{
    Platform_Work_QueueCallback *Callback;
    void *Data;
};

struct Platform_Work_Queue 
{
    u32 volatile CompletionGoal;
    u32 volatile CompletionCount;

    u32 volatile NextEntryToWrite;
    u32 volatile NextEntryToRead;
    HANDLE SemaphoreHandle;

    Platform_Work_QueueEntry Entries[256];
};

internal void
Win32AddEntry(Platform_Work_Queue *Queue, Platform_Work_QueueCallback *Callback, void *Data) 
{
    // TODO: Switch to InterlockedCompareExchange eventually
    // so that any thread can add?
    u32 NewNextEntryToWrite = (Queue->NextEntryToWrite + 1) % arraycount(Queue->Entries);
    Assert(NewNextEntryToWrite != Queue->NextEntryToRead);
    Platform_Work_QueueEntry *Entry = Queue->Entries + Queue->NextEntryToWrite;
    Entry->Callback = Callback;    Entry->Data = Data;
    ++Queue->CompletionGoal;
    _WriteBarrier();
    Queue->NextEntryToWrite = NewNextEntryToWrite;
    ReleaseSemaphore(Queue->SemaphoreHandle, 1, 0);
}

internal bool32
Win32DoNextWorkQueueEntry(Platform_Work_Queue *Queue) 
{
    b32 shouldSleep = false;

    u32 OriginalNextEntryToRead = Queue->NextEntryToRead;
    u32 NewNextEntryToRead = (OriginalNextEntryToRead + 1) % arraycount(Queue->Entries);
    if (OriginalNextEntryToRead != Queue->NextEntryToWrite)
    {
        u32 Index = InterlockedCompareExchange((LONG volatile *)&Queue->NextEntryToRead,
                                               NewNextEntryToRead,
                                               OriginalNextEntryToRead);
        if(Index == OriginalNextEntryToRead)
        {        
            Platform_Work_QueueEntry Entry = Queue->Entries[Index];
            Entry.Callback(Queue, Entry.Data);
            InterlockedIncrement((LONG volatile *)&Queue->CompletionCount);
        }
    } else {
        shouldSleep = true;
    }

    return shouldSleep;
}

internal void
win32_complete_all_work(Platform_Work_Queue *Queue) 
{
    while(Queue->CompletionGoal != Queue->CompletionCount) 
    {
        Win32DoNextWorkQueueEntry(Queue);
    }

    Queue->CompletionGoal = 0;
    Queue->CompletionCount = 0;
}

DWORD WINAPI
ThreadProc(LPVOID lpParameter) 
{
    Platform_Work_Queue *Queue = (Platform_Work_Queue *)lpParameter;

    for(;;) 
    {
        if (Win32DoNextWorkQueueEntry(Queue)) 
        {
            WaitForSingleObjectEx(Queue->SemaphoreHandle, INFINITE, FALSE);
        }
    }
}

internal void
win32_make_queue(Platform_Work_Queue *Queue, u32 ThreadCount) 
{
    Queue->CompletionGoal = 0;
    Queue->CompletionCount = 0;

    Queue->NextEntryToWrite = 0;
    Queue->NextEntryToRead = 0;

    u32 InitialCount = 0;
    Queue->SemaphoreHandle = CreateSemaphoreEx(0,
                                               InitialCount,
                                               ThreadCount,
                                               0, 0, SEMAPHORE_ALL_ACCESS);
    for(u32 ThreadIndex = 0;
        ThreadIndex < ThreadCount;
        ++ThreadIndex) 
    {
        DWORD ThreadID;
        HANDLE ThreadHandle = CreateThread(0, 0, ThreadProc, Queue, 0, &ThreadID);
        CloseHandle(ThreadHandle);
    }
}

#if __DEVELOPER
DEBUG_PLATFORM_EXECUTE_SYSTEM_COMMAND(win32_execute_system_command)
{
    Debug_Executing_Process result = {};

    STARTUPINFO startup_info = {};
    startup_info.cb = sizeof(startup_info);
    startup_info.dwFlags = STARTF_USESHOWWINDOW;
    startup_info.wShowWindow = SW_HIDE;

    PROCESS_INFORMATION process_info = {};
    if (CreateProcessA(command, commandline, 0, 0, FALSE, 0, 0, path, &startup_info, &process_info)) {
         static_assert(sizeof(result.os_handle) >= sizeof(process_info.hProcess));
        *(HANDLE *)&result.os_handle = process_info.hProcess;
    } else {
        DWORD error_code = GetLastError();
        *(HANDLE *)&result.os_handle = INVALID_HANDLE_VALUE;
    }

    return result;
}
#endif

DEBUG_PLATFORM_GET_PROCESS_STATE(win32_get_process_state)
{
    Debug_Process_State result = {};

    HANDLE hProcess = *(HANDLE *)&process.os_handle;
    if (hProcess != INVALID_HANDLE_VALUE)
    {
        result.started_successfully = true;

        if (WaitForSingleObject(hProcess, 0) == WAIT_OBJECT_0)
        {
            DWORD return_code;
            GetExitCodeProcess(hProcess, &return_code);
            result.return_code = return_code;
            CloseHandle(hProcess);
        }
        else
        {
            result.is_running = true;
        }
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
        Assert(!"coinit failed"); // @TODO: error-handling

    IXAudio2 *xaudio = 0;
    if (FAILED(hr = XAudio2Create(&xaudio, 0, XAUDIO2_DEFAULT_PROCESSOR)))
        Assert(!"XAudio init failed"); // @TODO: error-handling

    IXAudio2MasteringVoice *master_voice = 0;
    if (FAILED(hr = xaudio->CreateMasteringVoice(&master_voice)))
        Assert(!"master voice creation failed"); // @TODO: error-handling

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
#if __DEVELOPER
    g_show_cursor = true;
#endif

    WNDCLASSA wnd_class = {};
    wnd_class.style             = CS_HREDRAW|CS_VREDRAW|CS_OWNDC;
    wnd_class.lpfnWndProc       = win32_window_callback;
    wnd_class.hInstance         = hinst;
    wnd_class.hCursor           = LoadCursorA(0, IDC_ARROW);
    wnd_class.hbrBackground     = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wnd_class.lpszClassName     = "GameWindowClass";
    RegisterClassA(&wnd_class);

    HWND hwnd = CreateWindowExA(0, wnd_class.lpszClassName, "Game",
                                WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                                0, 0, hinst, 0);
    Assert(hwnd);
    return hwnd;
}

internal void
win32_unload_code(Win32_Loaded_Code *loaded)
{
    if (loaded->dll)
    {
        // @TODO: Currently, we never unload libraries, because we may still be pointing to strings that are inside them
        // (despite our best efforts). Should we just make "never unload" be the policy?
        
        // FreeLibrary(GameCode->GameCodeDLL);
        loaded->dll = 0;
    }
    
    loaded->is_valid = false;
    zeroarray(loaded->functions, loaded->function_count);
}

internal PLATFORM_GET_LAST_WRITE_TIME(win32_get_last_write_time_)
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
    String b = wrapz(filename);
    if (unique == 0) {
        snprintf(dst, dst_count, "%.*s%.*s", (int)a.count, a.data, (int)b.count, b.data);
    } else {
        snprintf(dst, dst_count, "%.*s%u%.*s", (int)a.count, a.data, unique, (int)b.count, b.data);
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

            if (CopyFile(src_dll_name, temp_dll_name, FALSE)) {
                break;
            }
        }

        loaded->dll = LoadLibraryA(temp_dll_name);
        if (loaded->dll)
        {
            loaded->is_valid = true;
            for (u32 i = 0; i < loaded->function_count; ++i)
            {
                void *function = (void *)GetProcAddress(loaded->dll, loaded->function_names[i]);
                if (function) {
                    loaded->functions[i] = function;
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
    return __rdtsc();
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

PLATFORM_GET_SYSTEM_TIME(win32_get_system_time)
{
    Platform_Time result = {};
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

PLATFORM_LIST_FILES(win32_list_files)
{
    Platform_File_List result = {};

    WIN32_FIND_DATA finddata;
    HANDLE hfind = INVALID_HANDLE_VALUE;
    DWORD dwError=0;

    umm len = string_length(path);
    char wildcard[MAX_PATH];
    copy_array(path, wildcard, len);
    Assert(len + 2 < MAX_PATH);
    wildcard[len] = '/';
    wildcard[len+1] = '*';
    wildcard[len+2] = 0;

    hfind = FindFirstFile(wildcard, &finddata);

    if (hfind != INVALID_HANDLE_VALUE) {
        do {
            Platform_File_Info info = {};

            if (finddata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                info.type = TYPE_DIRECTORY;
                copyz(finddata.cFileName, info.filename);
            } else {
                info.type = TYPE_FILE;
                copyz(finddata.cFileName, info.filename);
            }

            result.infos[result.count++] = info;
        }
        while (FindNextFile(hfind, &finddata) != 0);

        dwError = GetLastError();
        if (dwError != ERROR_NO_MORE_FILES) {
            Assert(0);
        }

        FindClose(hfind);
    } else {
        Assert(0);
    }

    return result;
}

#if __DEVELOPER
int main()
#else
int WINAPI
WinMain(HINSTANCE hinst, HINSTANCE deprecated, LPSTR cmd, int show_cmd) 
#endif
{
    Win32_State *state = &g_win32_state;

    win32_get_exe_filepath(state);

    char game_dll_path[WIN32_MAX_PATH_LENGTH];
    win32_build_exe_path_filename(state, "game.dll", game_dll_path, sizeof(game_dll_path));

    char renderer_dll_path[WIN32_MAX_PATH_LENGTH];
    win32_build_exe_path_filename(state, "win32_opengl.dll", renderer_dll_path, sizeof(renderer_dll_path));

    char code_lock_path[WIN32_MAX_PATH_LENGTH];
    win32_build_exe_path_filename(state, "lock.tmp", code_lock_path, sizeof(code_lock_path));

    // @NOTE: Set the Windows schedular granularity to 1ms so that our Sleep() can be more granular.
    UINT desired_schedular_ms = 1;
    b32 sleep_is_granular = (timeBeginPeriod(desired_schedular_ms) == TIMERR_NOERROR);
    u64 cpu_timer_freq = win32_estimate_cpu_timer_frequency();
    f32 inv_cpu_timer_freq = 1.0 / cpu_timer_freq;


    Platform_Work_Queue high_priority_queue = {};
    win32_make_queue(&high_priority_queue, 6);

    Platform_Work_Queue low_priority_queue = {};
    win32_make_queue(&low_priority_queue, 2);



#if __DEVELOPER
    HINSTANCE hinst = GetModuleHandleA(0);
#endif
    HWND hwnd = win32_create_window(hinst);

    if (hwnd) 
    {
        state->default_window_handle = hwnd;

#if !__DEVELOPER
        win32_toggle_fullscreen(hwnd);
#endif

        HDC renderer_hdc = GetDC(hwnd);

        b32 renderer_was_reloaded = false;
        Win32_Renderer_Function_Table renderer_functions = {};
        Win32_Loaded_Code renderer_code = {};
        renderer_code.transient_dll_name = "renderer_temp.dll";
        renderer_code.dll_full_path = renderer_dll_path;
        renderer_code.lock_full_path = code_lock_path;
        renderer_code.function_count = arraycount(win32_renderer_function_table_names);
        renderer_code.functions = (void **)&renderer_functions;
        renderer_code.function_names = win32_renderer_function_table_names;
        win32_load_code(state, &renderer_code);
        if (!renderer_code.is_valid) {
            // @TODO: Error Handling.
            Assert(0);
        }
        umm renderer_memory_size = GB(1);
        void *renderer_memory = VirtualAlloc(0, renderer_memory_size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
        Memory_Arena renderer_arena = {};
        init_arena(&renderer_arena, renderer_memory, renderer_memory_size);
        Platform_Renderer *renderer = renderer_functions.load_renderer(renderer_hdc, MB(50), &renderer_arena); 

        win32_load_xinput();
        //HRESULT init_audio_result = win32_init_audio();


#if __DEVELOPER
        LPVOID base_address = (LPVOID)TB(2);
#else
        LPVOID base_address = 0;
#endif

        Game_Memory game_memory = {};
        umm total_memory_size = GB(2);
        game_memory.total_memory_size = total_memory_size;
        state->game_memory = VirtualAlloc(base_address, total_memory_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
        state->game_memory_total_size = total_memory_size;

        game_memory.total_memory = (u8 *)state->game_memory;
        game_memory.high_priority_queue = &high_priority_queue;
        game_memory.low_priority_queue = &low_priority_queue;

        game_memory.os.add_entry                = Win32AddEntry;
        game_memory.os.complete_all_work        = win32_complete_all_work;
        game_memory.os.get_working_directory    = win32_get_working_directory;
        game_memory.os.list_files               = win32_list_files;
        game_memory.os.open_file                = win32_open_file;
        game_memory.os.close_file               = win32_close_file;
        game_memory.os.read_from_file           = win32_read_from_file;
        game_memory.os.read_entire_file         = win32_read_entire_file;
        game_memory.os.copy_file                = win32_copy_file;
        game_memory.os.get_file_size            = win32_get_file_size;
        game_memory.os.free_memory              = win32_free_memory;
        game_memory.os.get_system_time          = win32_get_system_time;
        game_memory.os.read_cpu_timer           = win32_read_cpu_timer;
        game_memory.os.get_last_write_time      = win32_get_last_write_time_;
        game_memory.os.cpu_timer_frequency      = cpu_timer_freq;
#if __DEVELOPER
        game_memory.os.debug_platform_execute_system_command = win32_execute_system_command;
        game_memory.os.debug_platform_get_process_state      = win32_get_process_state;
#endif
        os = game_memory.os;


        u32 monitor_refresh_rate = (u32)GetDeviceCaps(renderer_hdc, VREFRESH);
        f32 desired_dt = 1.0f / (f32)monitor_refresh_rate;


        Input input = {};
        u8 win32_keycode_map[256];
        win32_map_keycode_to_hid_key_code(win32_keycode_map);

        Event_Queue event_queue = {};

        if (game_memory.total_memory)
        {
            u64 last_cpu_timer = win32_read_cpu_timer();

            Win32_Game_Function_Table game = {};
            Win32_Loaded_Code game_code = {};
            game_code.transient_dll_name = "game_temp.dll";
            game_code.dll_full_path      = game_dll_path;
            game_code.lock_full_path     = code_lock_path;
            game_code.function_count     = arraycount(win32_game_function_table_names);
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
                        b32 was_down = ((msg.lParam & (1 << 30)) != 0);
                        b32 is_down  = ((msg.lParam & (1UL << 31)) == 0);
                        b32 alt      = (msg.lParam & (1 << 29));
                        u8 slot      = win32_keycode_map[vk_code];

                        if (was_down != is_down) {
                            if (event_queue.next_idx < arraycount(event_queue.events)) {
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
                    char buf[256];
                    snprintf(buf, sizeof(buf), "%.2f, %.2f\n", input.mouse.position.x, input.mouse.position.y);
                    OutputDebugStringA(buf);

                    win32_process_mouse_click(VK_LBUTTON, mouse);
                    win32_process_mouse_click(VK_MBUTTON, mouse);
                    win32_process_mouse_click(VK_RBUTTON, mouse);
                }

                DWORD result;    
                for (DWORD idx = 0; idx < XUSER_MAX_COUNT; idx++) 
                {
                    XINPUT_STATE xinput_state;
                    zerostruct(&xinput_state, XINPUT_STATE);
                    result = xinput_get_state(idx, &xinput_state);
                    win32_xinput_handle_deadzone(&xinput_state);
                    if (result == ERROR_SUCCESS) {

                    } 
                    else {
                        // TODO: Diagnostic
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
                    win32_complete_all_work(&high_priority_queue);
                    win32_complete_all_work(&low_priority_queue);
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
                    renderer = renderer_functions.load_renderer(renderer_hdc, MB(50), &renderer_arena); 
                }
            }
        } else {
            // @TODO: Error handling.
        }
    } else {
        // @TODO: Error handling.
    }

    return 0;
}
