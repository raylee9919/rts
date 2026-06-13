// Copyright Seong Woo Lee. All Rights Reserved.

// @Temporary
//
void add_work(Work_Queue* queue, Work_Callback* callback, void* param) {
    // @Todo: Single producer implementation atm. We gonna have to switch to 
    // cmpxchg if we want multiple producers, ultimately.
    u32 index = queue->index_to_write;
    u32 new_write_index = (queue->index_to_write + 1) % array_count(queue->works);

    assert(new_write_index != queue->index_to_read);

    queue->works[index].callback = callback;
    queue->works[index].param = param;
    _WriteBarrier();
    queue->index_to_write = new_write_index;
    queue->completion_goal++;

    HANDLE semaphore = win32_handle_from_os_handle(queue->semaphore);
    ReleaseSemaphore(semaphore, 1, 0);
}

bool do_work_or_should_sleep(Work_Queue* queue)
{
    bool should_sleep = false;

    u32 old_index = queue->index_to_read;
    u32 new_index = (old_index + 1) % array_count(queue->works);

    if (old_index != queue->index_to_write) {
        u32 index = InterlockedCompareExchange((LONG volatile *)&queue->index_to_read, new_index, old_index);
        if (index == old_index) {
            Work work = queue->works[index];
            work.callback(work.param);
            InterlockedIncrement(&queue->completion_count);
        }
    } else {
        should_sleep = true;
    }

    return should_sleep;
}

void complete_all_work(Work_Queue *queue) 
{
    while (queue->completion_count != queue->completion_goal) {
        do_work_or_should_sleep(queue);
    }

    queue->completion_count = 0;
    queue->completion_goal  = 0;
}

DWORD worker_thread_proc(LPVOID param) {
    Work_Queue *queue = (Work_Queue *)param;

    for (;;) {
        if (do_work_or_should_sleep(queue)) {
            HANDLE semaphore = win32_handle_from_os_handle(queue->semaphore);
            WaitForSingleObject(semaphore, INFINITE);
        }
    }
}

void init_work_queue(Work_Queue* queue, int num_threads) {
    if (num_threads == 0) {
        return;
    }

    HANDLE semaphore = CreateSemaphore(NULL, 0, num_threads, 0);
    queue->semaphore = os_handle_from_win32_handle(semaphore);

    for (int i = 0; i < num_threads; ++i) {
        DWORD tid;
        HANDLE thread_handle = CreateThread(NULL, 0, worker_thread_proc, queue, 0, &tid);
        CloseHandle(thread_handle);
    }
}

// Init
//
void os_init() {
    Arena* arena = arena_alloc();
    os = push_struct(arena, OS_State);
    os->arena = arena;

    // Events
    os->event_arena = arena_alloc();

    // Build virtual key code to 'OS_Key' table.
    for (u32 vk = 'A', k = KEY_A; vk <= 'Z'; ++vk, ++k) {
        os->vk_to_key[vk] = (OS_Key)k;
    }

    for (u32 vk = '0', k = KEY_0; vk <= '9'; ++vk, ++k) {
        os->vk_to_key[vk] = (OS_Key)k;
    }

    for (u32 vk = VK_F1, k = KEY_F1; vk <= VK_F24; ++vk, ++k) {
        os->vk_to_key[vk] = (OS_Key)k;
    }

    os->vk_to_key[VK_ESCAPE]        = KEY_ESC;
    os->vk_to_key[VK_OEM_3]         = KEY_TILDE;
    os->vk_to_key[VK_OEM_MINUS]     = KEY_MINUS;
    os->vk_to_key[VK_OEM_PLUS]      = KEY_EQUAL;
    os->vk_to_key[VK_BACK]          = KEY_BACKSPACE;
    os->vk_to_key[VK_TAB]           = KEY_TAB;
    os->vk_to_key[VK_SPACE]         = KEY_SPACE;
    os->vk_to_key[VK_RETURN]        = KEY_RETURN;
    os->vk_to_key[VK_CONTROL]       = KEY_CTRL;
    os->vk_to_key[VK_LCONTROL]      = KEY_CTRL;
    os->vk_to_key[VK_RCONTROL]      = KEY_CTRL;
    os->vk_to_key[VK_SHIFT]         = KEY_SHIFT;
    os->vk_to_key[VK_LSHIFT]        = KEY_SHIFT;
    os->vk_to_key[VK_RSHIFT]        = KEY_SHIFT;
    os->vk_to_key[VK_MENU]          = KEY_ALT;
    os->vk_to_key[VK_LMENU]         = KEY_ALT;
    os->vk_to_key[VK_RMENU]         = KEY_ALT;
    os->vk_to_key[VK_UP]            = KEY_UP;
    os->vk_to_key[VK_LEFT]          = KEY_LEFT;
    os->vk_to_key[VK_DOWN]          = KEY_DOWN;
    os->vk_to_key[VK_RIGHT]         = KEY_RIGHT;
    os->vk_to_key[VK_DELETE]        = KEY_DELETE;
    os->vk_to_key[VK_PRIOR]         = KEY_PAGE_UP;
    os->vk_to_key[VK_NEXT]          = KEY_PAGE_DOWN;
    os->vk_to_key[VK_HOME]          = KEY_HOME;
    os->vk_to_key[VK_END]           = KEY_END;
    os->vk_to_key[VK_OEM_2]         = KEY_SLASH;
    os->vk_to_key[VK_OEM_5]         = KEY_BACK_SLASH;
    os->vk_to_key[VK_OEM_PERIOD]    = KEY_PERIOD;
    os->vk_to_key[VK_OEM_COMMA]     = KEY_COMMA;
    os->vk_to_key[VK_OEM_7]         = KEY_QUOTE;
    os->vk_to_key[VK_OEM_4]         = KEY_LEFT_BRACKET;
    os->vk_to_key[VK_OEM_6]         = KEY_RIGHT_BRACKET;
    os->vk_to_key[VK_INSERT]        = KEY_INSERT;
    os->vk_to_key[VK_OEM_1]         = KEY_SEMICOLON;
    os->vk_to_key[VK_PAUSE]         = KEY_PAUSE;
    os->vk_to_key[VK_CAPITAL]       = KEY_CAPS_LOCK;
    os->vk_to_key[VK_NUMLOCK]       = KEY_NUMS_LOCK;
    os->vk_to_key[VK_SCROLL]        = KEY_SCROLL_LOCK;
    os->vk_to_key[VK_APPS]          = KEY_MENU;

    // Numpad
    os->vk_to_key[VK_DIVIDE]        = KEY_NUM_DIVIDE;
    os->vk_to_key[VK_MULTIPLY]      = KEY_NUM_MULTIPLY;
    os->vk_to_key[VK_SUBTRACT]      = KEY_NUM_SUBTRACT;
    os->vk_to_key[VK_ADD]           = KEY_NUM_ADD;
    os->vk_to_key[VK_DECIMAL]       = KEY_NUM_DECIMAL;

    // Mouse
    os->vk_to_key[VK_LBUTTON]       = KEY_MOUSE_LEFT;
    os->vk_to_key[VK_RBUTTON]       = KEY_MOUSE_RIGHT;
    os->vk_to_key[VK_MBUTTON]       = KEY_MOUSE_MIDDLE;


    // Cache counter frequency.
    os->qpc_rcp_freq64 = 1.0 / (f64)os_counter_freq();
    os->qpc_rcp_freq32 = (f32)os->qpc_rcp_freq64;


    // Gather paths.
    {
        Utf8 binary_path = {};
        Utf8 appdata_path = {};
        {
            Temporary_Arena tmp = temporary_arena_begin(os->arena);

            {
                DWORD size = KB(32);
                u16 *buffer = push_array_noz(tmp.arena, u16, size);
                DWORD length = GetModuleFileNameW(0, (WCHAR *)buffer, size);
                binary_path = to_utf8(tmp.arena, utf16(buffer, length));
                binary_path = utf8_path_chop_last_slash(binary_path);
            }

            {
                DWORD size = KB(32);
                u16 *buffer = push_array_noz(tmp.arena, u16, size);
                if (SUCCEEDED(SHGetFolderPathW(0, CSIDL_APPDATA, 0, 0, (WCHAR *)buffer)))
                {
                    appdata_path = to_utf8(tmp.arena, utf16c(buffer));
                }
            }

            temporary_arena_end(tmp);
        }
        {
            os->binary_path  = utf8_copy(os->arena, binary_path);
            os->initial_path = os->binary_path;
            os->appdata_path = utf8_copy(os->arena, appdata_path);
        }
    }

    // @Temporary
    init_work_queue(&os->work_queue, os_query_core_count() - 1);
}


// Memory
//
void* os_reserve(u64 size) {
    void *result = VirtualAlloc(0, size, MEM_RESERVE, PAGE_READWRITE);
    return result;
}

bool os_commit(void* ptr, u64 size) {
    bool result = (VirtualAlloc(ptr, size, MEM_COMMIT, PAGE_READWRITE) != 0);
    return result;
}

void os_decommit(void* ptr, u64 size) {
    VirtualFree(ptr, size, MEM_DECOMMIT);
}

void os_release(void* ptr, u64 size) {
    // Size isn't required on Windows, but is required on other OSes.
    VirtualFree(ptr, 0, MEM_RELEASE);
}

void* os_heap_alloc(u64 size) {
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


// Counter
//   I'm well aware of other sorts of timer.. Bad naming? idk.
//
u64 os_counter() {
    LARGE_INTEGER li;
    QueryPerformanceCounter(&li);
    return li.QuadPart;
}

u64 os_counter_freq(void) {
    LARGE_INTEGER li;
    QueryPerformanceFrequency(&li);
    return li.QuadPart;
}

f32 os_counter_freq_rcp() {
    return os->qpc_rcp_freq32;
}

f64 os_counter_freq_rcp64() {
    return os->qpc_rcp_freq64;
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


// File
//
OS_Handle os_open_file(Utf8 path, OS_Access_Flags flags) {
    // https://stackoverflow.com/a/14469641
    //                          |                    When the file...
    // This argument:           |             Exists            Does not exist
    // -------------------------+------------------------------------------------------
    // CREATE_ALWAYS            |            Truncates             Creates
    // CREATE_NEW         +-----------+        Fails               Creates
    // OPEN_ALWAYS     ===| does this |===>    Opens               Creates
    // OPEN_EXISTING      +-----------+        Opens                Fails
    // TRUNCATE_EXISTING        |            Truncates              Fails
    //
    Temporary_Arena scratch = scratch_begin();

    OS_Handle result = {};
    Utf16 path16 = to_utf16(scratch.arena, path);

    // make access flags.
    DWORD access = 0;
    if (flags & OS_ACCESS_FLAG_READ)    access |= GENERIC_READ;
    if (flags & OS_ACCESS_FLAG_WRITE)   access |= GENERIC_WRITE;
    if (flags & OS_ACCESS_FLAG_APPEND)  access |= FILE_APPEND_DATA;
    if (flags & OS_ACCESS_FLAG_EXECUTE) access |= GENERIC_EXECUTE;

    // make share mode flags.
    DWORD share = 0;
    if (flags & OS_ACCESS_FLAG_SHARE_READ)  share |= FILE_SHARE_READ;
    if (flags & OS_ACCESS_FLAG_SHARE_WRITE) share |= FILE_SHARE_WRITE;

    SECURITY_ATTRIBUTES security = { sizeof(SECURITY_ATTRIBUTES), NULL, FALSE };

    // make creation disposition value.
    DWORD creation_disposition = OPEN_EXISTING;
    if (flags & OS_ACCESS_FLAG_WRITE)  creation_disposition = CREATE_ALWAYS;
    if (flags & OS_ACCESS_FLAG_APPEND) creation_disposition = OPEN_ALWAYS;

    HANDLE handle = CreateFileW((WCHAR*)path16.str, access, share, &security, 
                                creation_disposition, FILE_ATTRIBUTE_NORMAL, NULL);

    if (handle != INVALID_HANDLE_VALUE) {
        result = os_handle_from_win32_handle(handle);
    } else {
        DWORD error = GetLastError();
        (void)error;
    }

    scratch_end(scratch);
    return result;
}

void os_close_file(OS_Handle file) {
    HANDLE handle = win32_handle_from_os_handle(file);
    BOOL ok = CloseHandle(handle);
    (void)ok;
}

u64 os_read_file(OS_Handle file, u64 offset, u64 size, void* out) {
    assert(size > 0);

    u64 result = 0;
    HANDLE handle = win32_handle_from_os_handle(file);
    u64 file_size = 0;

    // clamp read size by file size.
    GetFileSizeEx(handle, (LARGE_INTEGER*)&file_size);
    assert(file_size >= offset);
    size = min(size, file_size - offset);

    while (result < size) {
        u64 size64 = size - result;
        u32 size32 = size64 > U32_MAX ? U32_MAX : (u32)size64;

        DWORD read_size = 0;

        OVERLAPPED overlapped = {};
        overlapped.Offset     = (u32)offset;
        overlapped.OffsetHigh = (u32)(offset >> 32);

        ReadFile(handle, (u8*)out + result, size32, &read_size, &overlapped);

        offset += read_size;
        result += read_size;

        if (read_size != size32) break;
    }

    return result;
}

bool os_delete_file(Utf8 path) {
    Temporary_Arena scratch = scratch_begin();
    Utf16 path16 = to_utf16(scratch.arena, path);
    bool result = DeleteFileW((WCHAR*)path16.str);
    scratch_end(scratch);
    return result;
}

bool os_copy_file(Utf8 dst, Utf8 src) {
    Temporary_Arena scratch = scratch_begin();
    Utf16 dst16 = to_utf16(scratch.arena, dst);
    Utf16 src16 = to_utf16(scratch.arena, src);
    BOOL fail_if_exists = FALSE;
    bool result = CopyFileW((WCHAR*)src16.str, (WCHAR*)dst16.str, fail_if_exists);
    scratch_end(scratch);
    return result;
}

File_Properties os_get_file_properties(OS_Handle file) {
    File_Properties result = {};

    HANDLE handle = win32_handle_from_os_handle(file);
    BY_HANDLE_FILE_INFORMATION info = {};

    if (GetFileInformationByHandle(handle, &info)) {
        u32 size_lo = info.nFileSizeLow;
        u32 size_hi = info.nFileSizeHigh;
        result.size = (((u64)size_hi) << 32) | (u64)size_lo;
        result.is_directory = info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
    }

    return result;
}

File_Properties os_get_file_properties(Utf8 path) {
    Temporary_Arena scratch = scratch_begin();

    WIN32_FIND_DATAW find_data = {};
    Utf16 path16 = to_utf16(scratch.arena, path);

    HANDLE handle = FindFirstFileW((WCHAR*)path16.str, &find_data);
    File_Properties result = {};

    if (handle != INVALID_HANDLE_VALUE) {
        result.size = (((u64)find_data.nFileSizeHigh) << 32) | (u64)find_data.nFileSizeLow;
        result.is_directory = find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
    } else {
        assert(!"X");
    }

    FindClose(handle);
    scratch_end(scratch);
    return result;
}

u64 os_get_file_size(OS_Handle file) {
    HANDLE handle = win32_handle_from_os_handle(file);
    u64 size = 0;
    GetFileSizeEx(handle, (LARGE_INTEGER*)&size);
    return size;
}

u64 os_get_file_size(Utf8 path) {
    return os_get_file_properties(path).size;
}

bool os_create_directory(Utf8 path) {
    Temporary_Arena scratch = scratch_begin();

    bool result = false;
    Utf16 path16 = to_utf16(scratch.arena, path);
    WIN32_FILE_ATTRIBUTE_DATA attrib;

    GetFileAttributesExW((WCHAR*)path16.str, GetFileExInfoStandard, &attrib);

    if (attrib.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        result = true;
    } else if (CreateDirectoryW((WCHAR*)path16.str, NULL)) {
        result = true;
    }

    scratch_end(scratch);
    return result;
}

bool os_directory_exists(Utf8 path) {
    Temporary_Arena scratch = scratch_begin();
    Utf16 path16 = to_utf16(scratch.arena, path);
    DWORD attrib = GetFileAttributesW((WCHAR *)path16.str);
    bool result = (attrib != INVALID_FILE_ATTRIBUTES) && (attrib & FILE_ATTRIBUTE_DIRECTORY);
    scratch_end(scratch);
    return result;
}


// GFX
//
OS_Modifiers os_get_modifiers() {
    OS_Modifiers result = 0;

    if (GetKeyState(VK_CONTROL) & 0x8000) result |= OS_MODIFIER_CTRL;
    if (GetKeyState(VK_SHIFT)   & 0x8000) result |= OS_MODIFIER_SHIFT;
    if (GetKeyState(VK_MENU)    & 0x8000) result |= OS_MODIFIER_ALT;

    return result;
}

LRESULT win32_window_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    LRESULT result = 0;

    OS_Handle window_handle = os_handle_from_hwnd(hwnd);

    switch(msg) 
    {
        default: 
        {
            result = DefWindowProcW(hwnd, msg, wparam, lparam);
        } break;

        case WM_SIZE:
        case WM_PAINT: 
        {
            PAINTSTRUCT paint;
            BeginPaint(hwnd, &paint);
            EndPaint(hwnd, &paint);
        } break;


        case WM_CLOSE: 
        {
            // https://learn.microsoft.com/en-us/windows/win32/learnwin32/closing-the-window
            // Guess I don't need WM_QUIT, WM_DESTROY ?
            OS_Event* event = os_push_event();
            event->kind   = OS_EVENT_WINDOW_CLOSE;
            event->window = window_handle;
        } break;


        case WM_CHAR: 
        case WM_SYSCHAR: 
        {
            // WM_CHAR is uncode (UTF-16). If you need full unicode codepoint, 
            // you need to maintain state to handle surrogate pairs.
            // @Todo: Win + . to test it.
            //
            if (wparam >= 0xd800 && wparam <= 0xdbff) {
                assert(!"Surrogate pairs not supported yet.");
            }

            u32 codepoint = wparam;

            OS_Event* event = os_push_event();
            event->kind      = OS_EVENT_TEXT;
            event->window    = window_handle;
            event->codepoint = codepoint;
        } break;


        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_KEYUP: 
        {
            OS_Event_Kind kind;
            bool          is_repeat = false;
            u16           repeat_count;
            OS_Key        key;

            // Extract
            s16 lo = HIWORD(lparam);
            int is_down  = !(lo & KF_UP);
            int was_down =  (lo & KF_REPEAT);

            // Press? Release?
            kind = is_down ? OS_EVENT_PRESS : OS_EVENT_RELEASE;

            // Repeat?
            if (is_down && was_down) is_repeat = true;

            // Extract repeat count.
            repeat_count = lparam & 0xffff;

            // Get OS agnostic key.
            key = os->vk_to_key[wparam];

            OS_Event* event = os_push_event();
            event->kind          = kind;
            event->window        = window_handle;
            event->key           = key;
            event->is_repeat     = is_repeat;
            event->repeat_count  = repeat_count;
        } break;

        
        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
        case WM_MBUTTONUP:
        {
            // @Todo: Extended button

            OS_Event_Kind kind;
            OS_Key        key = KEY_NULL;
            v2            position;

            // Press? Release?
            if (msg == WM_LBUTTONDOWN || msg == WM_RBUTTONDOWN || msg == WM_MBUTTONDOWN) {
                kind = OS_EVENT_PRESS;
                SetCapture(hwnd);
            } else {
                kind = OS_EVENT_RELEASE;
                ReleaseCapture();
            }

            // Key?
            if      (msg == WM_LBUTTONDOWN || msg == WM_LBUTTONUP) key = KEY_MOUSE_LEFT;
            else if (msg == WM_RBUTTONDOWN || msg == WM_RBUTTONUP) key = KEY_MOUSE_RIGHT;
            else if (msg == WM_MBUTTONDOWN || msg == WM_MBUTTONUP) key = KEY_MOUSE_MIDDLE;
            else assert(!"Undefined key.");

            // Mouse position?
            f32 x = (f32)(s16)LOWORD(lparam);
            f32 y = (f32)(s16)HIWORD(lparam);
            position = v2{x, y};

            OS_Event* event = os_push_event();
            event->kind     = kind;
            event->window   = window_handle;
            event->key      = key;
            event->position = position;
        } break;


        case WM_MOUSEMOVE:
        {
            f32 x = (f32)(s16)LOWORD(lparam);
            f32 y = (f32)(s16)HIWORD(lparam);
            v2 position = v2{x, y};

            OS_Event* event = os_push_event();
            event->kind     = OS_EVENT_MOUSE_MOVE;
            event->window   = window_handle;
            event->position = position;
        } break;


        case WM_MOUSEWHEEL:
        case WM_MOUSEHWHEEL:
        {
            // Get scroll delta.
            f32 delta = (f32)HIWORD(wparam) / (f32)WHEEL_DELTA;

            // Convert screen-space mouse position to client space.
            POINT p;
            p.x = (s32)GET_X_LPARAM(lparam);
            p.y = (s32)GET_Y_LPARAM(lparam);
            ScreenToClient(hwnd, &p);

            // Make vec2.
            v2 position = v2{(f32)p.x, (f32)p.y};

            OS_Event* event = os_push_event();
            event->kind     = OS_EVENT_SCROLL;
            event->window   = window_handle;
            event->position = position;

            if (msg == WM_MOUSEWHEEL)
                event->delta = v2{0.f, delta};
            else
                event->delta = v2{delta, 0.f};
        } break;


        case WM_SETCURSOR: {
            // @Todo:
        } break;

        case WM_KILLFOCUS: {
            // @Todo:
        } break;

        case WM_DPICHANGED: {
            // @Todo:
        } break;

        case WM_DROPFILES: {
            // @Todo:
        } break;
    }

    return result;
}

void os_gfx_init() {
    HINSTANCE hinst = GetModuleHandleW(0);

    WNDCLASSEXW wcex = {};
    {
        wcex.cbSize         = sizeof(wcex);
        wcex.style          = CS_HREDRAW | CS_VREDRAW;
        wcex.lpfnWndProc    = win32_window_proc;
        wcex.hInstance      = hinst;
        wcex.hIcon          = LoadIcon(hinst, L"Icon");;
        wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);;
        wcex.hbrBackground  = CreateSolidBrush(RGB(30, 20, 20));
        wcex.lpszClassName  = L"GFX-Class";
        wcex.hIconSm;
    }
    RegisterClassExW(&wcex);

    ShowCursor(TRUE);

    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
}

OS_Handle os_create_window(int x, int y, int w, int h, Utf8 name) {
    Temporary_Arena scratch = scratch_begin();
    defer(scratch_end(scratch));

    HINSTANCE hinst = GetModuleHandleW(0);

    HWND hwnd = CreateWindowExW(WS_EX_APPWINDOW, L"GFX-Class", (LPCWSTR)to_utf16(scratch.arena, name).str, 
                                WS_OVERLAPPEDWINDOW | WS_SIZEBOX | WS_VISIBLE, x, y, w, h, 
                                0, 0, hinst, 0);
    DragAcceptFiles(hwnd, 1);

    OS_Handle result = os_handle_from_hwnd(hwnd);
    return result;
}

v2 os_get_window_size(OS_Handle window) {
    HWND hwnd = hwnd_from_os_handle(window);
    RECT rect;
    GetClientRect(hwnd, &rect);
    f32 x = rect.right - rect.left;
    f32 y = rect.bottom - rect.top;
    return v2{x, y};
}

v2 os_get_mouse_position(OS_Handle window) {
    HWND hwnd = hwnd_from_os_handle(window);
    POINT p;
    GetCursorPos(&p);
    ScreenToClient(hwnd, &p);
    return v2{(f32)p.x, (f32)p.y};
}



// Events
// 
void os_poll_events() {
    MSG msg;

    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) {

        } else {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    // 
    BYTE vk_state[256];
    GetKeyboardState(vk_state);
    memcpy(os->key_was_down, os->key_is_down, sizeof(os->key_is_down[0])*array_count(os->key_is_down));
    for (int vk = 0; vk < array_count(vk_state); ++vk) {
        OS_Key key = os->vk_to_key[vk];
        os->key_is_down[key] = vk_state[vk] & 0x80;
    }
}

OS_Event* os_push_event() {
    OS_Event* event = os->first_free_event;

    // Alloc
    if (event == NULL) {
        event = push_struct(os->event_arena, OS_Event);
    } else {
        sll_pop_front(os->first_free_event, os->last_free_event);
        memset(event, 0, sizeof(*event));
    }

    // Append to the list.
    dll_push_back(os->first_event, os->last_event, event);

    // Set modifiers.
    event->modifiers = os_get_modifiers();

    return event;
}

void os_remove_event(OS_Event* event) {
    dll_remove(os->first_event, os->last_event, event);
    sll_push_back(os->first_free_event, os->last_free_event, event);
    // @Todo: Freeing logic?
}

void os_clear_events() {
    list_for (os->first_event, event) {
        os_remove_event(event);
    }
}


// Main Entry
//
#if !BUILD_NO_ENTRY
int win32_main_entry() {
    return main_entry(0, NULL);
}

//int wWinMain(HINSTANCE hinst, HINSTANCE deprecated, PWSTR cmd, int show_cmd) {
//    return win32_main_entry();
//}

int main(int argc, char **argv) {
    return win32_main_entry();
}
#endif
