// Copyright Seong Woo Lee. All Rights Reserved.

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
        os->key_table[vk] = (OS_Key)k;
    }

    for (u32 vk = '0', k = KEY_0; vk <= '9'; ++vk, ++k) {
        os->key_table[vk] = (OS_Key)k;
    }

    for (u32 vk = VK_F1, k = KEY_F1; vk <= VK_F24; ++vk, ++k) {
        os->key_table[vk] = (OS_Key)k;
    }

    os->key_table[VK_ESCAPE]        = KEY_ESC;
    os->key_table[VK_OEM_3]         = KEY_TILDE;
    os->key_table[VK_OEM_MINUS]     = KEY_MINUS;
    os->key_table[VK_OEM_PLUS]      = KEY_EQUAL;
    os->key_table[VK_BACK]          = KEY_BACKSPACE;
    os->key_table[VK_TAB]           = KEY_TAB;
    os->key_table[VK_SPACE]         = KEY_SPACE;
    os->key_table[VK_RETURN]        = KEY_RETURN;
    os->key_table[VK_CONTROL]       = KEY_CTRL;
    os->key_table[VK_LCONTROL]      = KEY_CTRL;
    os->key_table[VK_RCONTROL]      = KEY_CTRL;
    os->key_table[VK_SHIFT]         = KEY_SHIFT;
    os->key_table[VK_LSHIFT]        = KEY_SHIFT;
    os->key_table[VK_RSHIFT]        = KEY_SHIFT;
    os->key_table[VK_MENU]          = KEY_ALT;
    os->key_table[VK_LMENU]         = KEY_ALT;
    os->key_table[VK_RMENU]         = KEY_ALT;
    os->key_table[VK_UP]            = KEY_UP;
    os->key_table[VK_LEFT]          = KEY_LEFT;
    os->key_table[VK_DOWN]          = KEY_DOWN;
    os->key_table[VK_RIGHT]         = KEY_RIGHT;
    os->key_table[VK_DELETE]        = KEY_DELETE;
    os->key_table[VK_PRIOR]         = KEY_PAGE_UP;
    os->key_table[VK_NEXT]          = KEY_PAGE_DOWN;
    os->key_table[VK_HOME]          = KEY_HOME;
    os->key_table[VK_END]           = KEY_END;
    os->key_table[VK_OEM_2]         = KEY_SLASH;
    os->key_table[VK_OEM_5]         = KEY_BACK_SLASH;
    os->key_table[VK_OEM_PERIOD]    = KEY_PERIOD;
    os->key_table[VK_OEM_COMMA]     = KEY_COMMA;
    os->key_table[VK_OEM_7]         = KEY_QUOTE;
    os->key_table[VK_OEM_4]         = KEY_LEFT_BRACKET;
    os->key_table[VK_OEM_6]         = KEY_RIGHT_BRACKET;
    os->key_table[VK_INSERT]        = KEY_INSERT;
    os->key_table[VK_OEM_1]         = KEY_SEMICOLON;
    os->key_table[VK_PAUSE]         = KEY_PAUSE;
    os->key_table[VK_CAPITAL]       = KEY_CAPS_LOCK;
    os->key_table[VK_NUMLOCK]       = KEY_NUMS_LOCK;
    os->key_table[VK_SCROLL]        = KEY_SCROLL_LOCK;
    os->key_table[VK_APPS]          = KEY_MENU;

    // Numpad
    os->key_table[VK_DIVIDE]        = KEY_NUM_DIVIDE;
    os->key_table[VK_MULTIPLY]      = KEY_NUM_MULTIPLY;
    os->key_table[VK_SUBTRACT]      = KEY_NUM_SUBTRACT;
    os->key_table[VK_ADD]           = KEY_NUM_ADD;
    os->key_table[VK_DECIMAL]       = KEY_NUM_DECIMAL;

    // Mouse
    os->key_table[VK_LBUTTON]       = KEY_MOUSE_LEFT;
    os->key_table[VK_RBUTTON]       = KEY_MOUSE_RIGHT;
    os->key_table[VK_MBUTTON]       = KEY_MOUSE_MIDDLE;


    // Cache counter frequency.
    os->qpc_rcp_freq64 = 1.0 / (f64)os_counter_freq();
    os->qpc_rcp_freq32 = (f32)os->qpc_rcp_freq64;
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
OS_Handle os_handle_from_hwnd(HWND hwnd) {
    OS_Handle result = {};
    result.e[0] = (u64)hwnd;
    return result;
}

HWND hwnd_from_os_handle(OS_Handle handle) {
    return (HWND)handle.e[0];
}

HANDLE win32_handle_from_os_handle(OS_Handle handle) {
    return (HANDLE)handle.e[0];
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
            event->kind          = OS_EVENT_WINDOW_CLOSE;
            event->window_handle = window_handle;
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
            event->kind          = OS_EVENT_TEXT;
            event->window_handle = window_handle;
            event->codepoint     = codepoint;
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
            key = os->key_table[wparam];

            OS_Event* event = os_push_event();
            event->kind          = kind;
            event->window_handle = window_handle;
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
            event->kind          = kind;
            event->window_handle = window_handle;
            event->key           = key;
            event->position      = position;
        } break;


        case WM_MOUSEMOVE:
        {
            f32 x = (f32)(s16)LOWORD(lparam);
            f32 y = (f32)(s16)HIWORD(lparam);
            v2 position = v2{x, y};

            OS_Event* event = os_push_event();
            event->kind          = OS_EVENT_MOUSE_MOVE;
            event->window_handle = window_handle;
            event->position      = position;
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
            event->kind          = OS_EVENT_SCROLL;
            event->window_handle = window_handle;
            event->position      = position;

            if (msg == WM_MOUSEWHEEL)
                event->delta = v2{0.f, delta};
            else
                event->delta = v2{delta, 0.f};
        } break;


        case WM_SETCURSOR: {
            // @Temporary
            SetCursor(0);
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
        wcex.lpszClassName  = L"GFX-Class";
        wcex.hIconSm;
    }

    RegisterClassExW(&wcex);
}

OS_Handle os_open_window(int x, int y, int w, int h, Utf8 name) {
    Temporary_Arena scratch = scratch_begin();
    defer(scratch_end(scratch));

    HINSTANCE hinst = GetModuleHandleW(0);

    HWND hwnd = CreateWindowExW(WS_EX_APPWINDOW, L"GFX-Class", (LPCWSTR)to_utf16(scratch.arena, name).str, 
                                WS_OVERLAPPEDWINDOW | WS_SIZEBOX, x, y, w, h, 
                                0, 0, hinst, 0);
    DragAcceptFiles(hwnd, 1);

    OS_Handle result = os_handle_from_hwnd(hwnd);
    return result;
}


// Events
// 
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
    for (OS_Event* event = os->first_event, *next; event != NULL; event = next) {
        next = event->next;
        os_remove_event(event);
    }
}


// Main Entry
//
#if !BUILD_NO_ENTRY
int win32_main_entry() {
    return main_entry(0, NULL);
}

int wWinMain(HINSTANCE hinst, HINSTANCE deprecated, PWSTR cmd, int show_cmd) {
    return win32_main_entry();
}
#endif
