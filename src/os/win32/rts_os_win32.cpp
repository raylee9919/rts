/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Seong Woo Lee $
   $Notice: (C) Copyright %s by Seong Woo Lee. All Rights Reserved. $
   ======================================================================== */

// --------------------------------------
// @Note: Win32 Handle Cast Functions
internal HANDLE
to_win32_handle(Os_Handle handle)
{
    static_assert(sizeof(HANDLE) == sizeof(handle.e[0]));
    HANDLE result = (HANDLE)handle.e[0];
    return result;
}

internal Os_Handle
to_os_handle(HANDLE handle)
{
    Os_Handle result = {};
    result.e[0] = (u64)handle;
    return result;
}

// --------------------------------------
// @Note: System Info
internal
OS_QUERY_PAGE_SIZE(win32_query_page_size)
{
    SYSTEM_INFO info;
    GetSystemInfo(&info);
    return info.dwPageSize;
}

internal
OS_CARET_BLINK_TIME(win32_caret_blink_time)
{
    UINT val = GetCaretBlinkTime();
    u32 result = val;
    return result;
}

internal
OS_STRING_FROM_SYSTEM_PATH_KIND(win32_string_from_system_path_kind)
{
    Temporary_Arena scratch = scratch_begin();
    Utf8 result = {};

    switch(path)
    {
        case OS_SYSTEM_PATH_KIND_INITIAL: {
            result = os->initial_path;
        } break;

        case OS_SYSTEM_PATH_KIND_CURRENT: {
            DWORD length = GetCurrentDirectoryW(0, 0);
            u16 *memory = push_array(scratch.arena, u16, length + 1);
            length = GetCurrentDirectoryW(length + 1, (WCHAR *)memory);
            result = to_utf8(arena, utf16(memory, length));
        } break;

        case OS_SYSTEM_PATH_KIND_BINARY: {
            result = os->binary_path;
        } break;

        case OS_SYSTEM_PATH_KIND_APPDATA: {
            result = os->appdata_path;
        } break;

        default: {
            Assert(! "invalid default case.");
        } break;
    }

    scratch_end(scratch);
    return result;
}

internal Os_File_Attributes
win32_file_attributes_from_find_data(WIN32_FIND_DATAW find_data)
{
    Os_File_Attributes result = {};

    if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
    { result.flags |= OS_FILE_FLAG_DIRECTORY; }

    result.size = (((u64)find_data.nFileSizeHigh) << 32) | (find_data.nFileSizeLow);
    result.last_modified = (((u64)find_data.ftLastWriteTime.dwHighDateTime) << 32) | (find_data.ftLastWriteTime.dwLowDateTime);
    return result;
}

internal
OS_ATTRIBUTES_FROM_FILE_PATH(win32_attributes_from_file_path)
{
    Temporary_Arena scratch = scratch_begin();

    WIN32_FIND_DATAW find_data = {};
    Utf16 path16 = to_utf16(scratch.arena, path);
    HANDLE handle = FindFirstFileW((WCHAR *)path16.str, &find_data);
    FindClose(handle);
    Os_File_Attributes result = win32_file_attributes_from_find_data(find_data);

    scratch_end(scratch);
    return result;
}

// ----------------------------------------
// @Note: File Iterator
OS_FILE_ITERATOR_BEGIN(win32_file_iterator_begin)
{
    Win32_File_Find_Data *file_find_data = push_struct(arena, Win32_File_Find_Data);
    Os_File_Iterator *it = (Os_File_Iterator *)file_find_data;
    path = utf8_skip_chop_whitespace(path);
    path = utf8_path_chop_last_slash(path);
    if (path.len != 0)
    {
        Temporary_Arena tmp = temporary_arena_begin(arena);
        path = utf8f(tmp.arena, "%S*", path);
        Utf16 path16 = to_utf16(tmp.arena, path);
        file_find_data->handle = FindFirstFileW((WCHAR *)path16.str, &file_find_data->find_data);
        temporary_arena_end(tmp);
    }
    return it;
}

internal
OS_FILE_ITERATOR_NEXT(win32_file_iterator_next)
{
    b32 result = 0;
    Win32_File_Find_Data *file_find_data = (Win32_File_Find_Data *)it;
    WIN32_FIND_DATAW find_data = {};

    for (;;)
    {
        // @Note: first, check initial results from FindFirstFile (dumb Windows API... never
        //        have 2 entry points (and thus caller codepaths) to return the same stuff!)
        b32 first_was_returned = 0;
        if (! file_find_data->returned_first)
        {
            result = (file_find_data->handle != 0 && file_find_data->handle != INVALID_HANDLE_VALUE);
            find_data = file_find_data->find_data;
            file_find_data->returned_first = 1;
            first_was_returned = 1;
        }

        // @Note: if we didn't return the first, OR the first was not good, then proceed to FindNextFile
        if (first_was_returned == 0)
        {
            result = FindNextFileW(file_find_data->handle, &find_data);
        }

        // rjf: check for filename validity. if it's invalid, skip.
        b32 filename_is_invalid = (find_data.cFileName[0] == '.' &&
                                   (find_data.cFileName[1] == 0 ||
                                    find_data.cFileName[1] == '.'));
        if (result == 0 || filename_is_invalid == 0)
        {
            break;
        }
    }

    // @Note: fill output
    if (result != 0)
    {
        Utf16 name16 = {0};
        name16.str = (u16 *)find_data.cFileName;
        name16.len = 0;
        for (u64 idx = 0; idx < MAX_PATH; idx += 1)
        {
            if (find_data.cFileName[idx] == 0)
            {
                break;
            }
            name16.len++;
        }
        zero_struct(out_info);
        out_info->name = to_utf8(arena, name16);
        out_info->attributes = win32_file_attributes_from_find_data(find_data);
    }

    return result;
}

internal
OS_FILE_ITERATOR_END(win32_file_iterator_end)
{
    Win32_File_Find_Data *file_find_data = (Win32_File_Find_Data *)it;
    FindClose(file_find_data->handle);
}


// --------------------------------------
// @Note: Memory
internal
OS_RESERVE(win32_memory_reserve)
{
    void *result = VirtualAlloc(0, size, MEM_RESERVE, PAGE_READWRITE);
    return result;
}

internal
OS_COMMIT(win32_memory_commit)
{
    b32 result = (VirtualAlloc(ptr, size, MEM_COMMIT, PAGE_READWRITE) != 0);
    return result;
}

internal
OS_DECOMMIT(win32_memory_decommit)
{
    VirtualFree(ptr, size, MEM_DECOMMIT);
}

internal
OS_RELEASE(win32_memory_release)
{
    // @Note: size is not required in Windows, but necessary in other OSes.
    VirtualFree(ptr, 0, MEM_RELEASE);
}


// --------------------------------------
// @Note: File
internal
OS_FILE_IS_VALID(win32_file_is_valid)
{
    HANDLE handle = to_win32_handle(file);
    b32 result = (handle != INVALID_HANDLE_VALUE);
    return result;
}

internal
OS_FILE_OPEN(win32_file_open)
{
    Temporary_Arena scratch = scratch_begin();

    Utf16 path16 = to_utf16(scratch.arena, path);

    DWORD desired_access = 0;
    if (flags & OS_FILE_ACCESS_READ)  
    { desired_access |= GENERIC_READ; }

    if (flags & OS_FILE_ACCESS_WRITE) 
    { desired_access |= GENERIC_WRITE; }

    DWORD share_mode = 0;
    if (flags & OS_FILE_ACCESS_SHARED) 
    { share_mode = FILE_SHARE_READ; }

    SECURITY_ATTRIBUTES security_attr = {};
    {
        security_attr.nLength              = sizeof(SECURITY_ATTRIBUTES);
        security_attr.lpSecurityDescriptor = NULL;
        security_attr.bInheritHandle       = FALSE;
    };

    DWORD creation_disposition = 0;
    if (flags & OS_FILE_ACCESS_CREATE_NEW)
    { creation_disposition = CREATE_ALWAYS; }
    else
    { creation_disposition = OPEN_EXISTING; }

    DWORD flags_and_attributes = 0;
    HANDLE template_file = NULL;

    HANDLE file = CreateFileW((LPCWSTR)path16.str, desired_access, share_mode, &security_attr,
                              creation_disposition, flags_and_attributes, template_file);

    Os_Handle result = {};
    if (file != INVALID_HANDLE_VALUE)
    { result = to_os_handle(file); }

    scratch_end(scratch);
    return result;
}

internal
OS_FILE_CLOSE(win32_file_close)
{
    HANDLE handle = to_win32_handle(file);
    if (handle != INVALID_HANDLE_VALUE)
    { CloseHandle(handle); }
}

internal
OS_FILE_SIZE(win32_file_size)
{
    HANDLE handle = to_win32_handle(file);
    if (handle == INVALID_HANDLE_VALUE)
    { Assert(0); } // @Todo: Error-Handling.
    LARGE_INTEGER file_size;
    GetFileSizeEx(handle, &file_size);
    u64 result = file_size.QuadPart;
    return result;
}

internal
OS_FILE_READ(win32_file_read)
{
    u64 result = 0;

    HANDLE handle = to_win32_handle(file);

    LARGE_INTEGER off_li = {};
    off_li.QuadPart = 0;
    if (handle == INVALID_HANDLE_VALUE)
    {
        Assert(0);
    }
    else if (SetFilePointerEx(handle, off_li, 0, FILE_BEGIN))
    {
        u64 bytes_to_read = size;
        u64 bytes_actually_read = 0;

        u8 *ptr = (u8 *)dst;
        u8 *opl = ptr + bytes_to_read;

        for(;;)
        {
            u64 unread = (u64)(opl - ptr);
            DWORD to_read = (DWORD)(min(unread, U32_MAX));
            DWORD did_read = 0;
            if(! ReadFile(handle, ptr, to_read, &did_read, 0))
            {
                break;
            }
            ptr += did_read;
            result += did_read;
            if (ptr >= opl)
            { break; }
        }
    }

    return result;
}

internal
OS_FILE_DELETE(win32_file_delete)
{
    Temporary_Arena scratch = scratch_begin();
    Utf16 path16 = to_utf16(scratch.arena, path);
    DeleteFileW((WCHAR *)path16.str);
    scratch_end(scratch);
}

internal
OS_FILE_MOVE(win32_file_move)
{
    Temporary_Arena scratch = scratch_begin();
    Utf16 dst_path16 = to_utf16(scratch.arena, dst_path);
    Utf16 src_path16 = to_utf16(scratch.arena, src_path);
    MoveFileW((WCHAR *)src_path16.str, (WCHAR *)dst_path16.str);
    scratch_end(scratch);
}

internal
OS_FILE_COPY(win32_file_copy)
{
    Temporary_Arena scratch = scratch_begin();
    Utf16 dst_path16 = to_utf16(scratch.arena, dst_path);
    Utf16 src_path16 = to_utf16(scratch.arena, src_path);
    b32 result = CopyFileW((WCHAR *)src_path16.str, (WCHAR *)dst_path16.str, 0);
    scratch_end(scratch);
    return result;
}

internal
OS_MAKE_DIRECTORY(win32_make_directory)
{
    Temporary_Arena scratch = scratch_begin();
    Utf16 path16 = to_utf16(scratch.arena, path);
    b32 result = true;
    if (! CreateDirectoryW((WCHAR *)path16.str, 0))
    {
        DWORD error = GetLastError();
        if (error != ERROR_ALREADY_EXISTS)
        { result = false; }
    }
    scratch_end(scratch);
    return result;
}

// --------------------------------------
// @Note: Abort
internal
OS_ABORT(win32_abort)
{
    ExitProcess(1);
}

// --------------------------------------
// @Note: Event Poll
internal
OS_EVENT_POLL(win32_event_poll)
{
    BYTE vk_state[256];
    GetKeyboardState(vk_state);

    zero_memory(os->key_toggled, sizeof(os->key_toggled[0]*array_count(os->key_toggled)));

    for (u32 vk = 0; vk < array_count(vk_state); ++vk)
    {
        Os_Key key = os->key_table[vk];
        b32 key_is_down_old = os->key_is_down[key];
        os->key_is_down[key] = (vk_state[vk] & 0x80) ? 1 : 0;
        os->key_toggled[key] = (os->key_is_down[key] != key_is_down_old);
    }
}

// --------------------------------------
// @Note: Performance Counter
internal
OS_PERF_COUNTER(win32_perf_counter)
{
    LARGE_INTEGER value;
    QueryPerformanceCounter(&value);
    return value.QuadPart;
}

internal u64
win32_perf_counter_frequency(void)
{
    LARGE_INTEGER value;
    QueryPerformanceFrequency(&value);
    return value.QuadPart;
}


// --------------------------------------
// @Note: Time
internal
OS_DATE_TIME_CURRENT(win32_date_time_current)
{
    SYSTEMTIME st = {};
    GetSystemTime(&st);
    Date_Time result = {};
    {
        result.year         = (u16)st.wYear;
        result.month        = (u8)st.wMonth;
        result.day_of_week  = (u8)st.wDayOfWeek;
        result.day          = (u8)st.wDay;
        result.hour         = (u8)st.wHour;
        result.minute       = (u8)st.wMinute;
        result.second       = (u8)st.wSecond;
        result.milliseconds = (u16)st.wMilliseconds;
    }
    return result;
}


// --------------------------------------
// @Note: Init
internal
OS_INIT(os_win32_init)
{
    // @Hack:
    static_assert(sizeof(OS) <= 4096);
    os = (OS *)VirtualAlloc(0, 4096, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);

    // ---------------------------------------------
    // @Note: init functions.
    os->file_is_valid                   = win32_file_is_valid;
    os->file_open                       = win32_file_open;
    os->file_close                      = win32_file_close;
    os->file_size                       = win32_file_size;
    os->file_read                       = win32_file_read;
    os->file_delete                     = win32_file_delete;
    os->file_move                       = win32_file_move;
    os->file_copy                       = win32_file_copy;
    os->make_directory                  = win32_make_directory;

    os->query_page_size                 = win32_query_page_size;
    os->caret_blink_time                = win32_caret_blink_time;
    os->string_from_system_path_kind    = win32_string_from_system_path_kind;
    os->attributes_from_file_path       = win32_attributes_from_file_path;

    os->file_iterator_begin             = win32_file_iterator_begin;
    os->file_iterator_next              = win32_file_iterator_next;
    os->file_iterator_end               = win32_file_iterator_end;

    os->memory_reserve                  = win32_memory_reserve;
    os->memory_commit                   = win32_memory_commit;
    os->memory_decommit                 = win32_memory_decommit;
    os->memory_release                  = win32_memory_release;

    os->abort                           = win32_abort;

    os->event_poll                      = win32_event_poll;

    os->perf_counter                    = win32_perf_counter;
    os->perf_counter_freq               = win32_perf_counter_frequency();
    os->perf_counter_freq_inv64         = (1.0 / (f64)os->perf_counter_freq);
    os->perf_counter_freq_inv           = (1.0 / (f32)os->perf_counter_freq);

    os->date_time_current               = win32_date_time_current;

    os->sleep_is_granular               = (timeBeginPeriod(1) == TIMERR_NOERROR);

    // @Note: Main Arena
    os->arena   = arena_alloc();

    // @Note: Event
    os->event_arena = arena_alloc();
    os->event_sentinel = push_struct(os->event_arena, Os_Event);
    {
        os->event_sentinel->next = os->event_sentinel;
        os->event_sentinel->prev = os->event_sentinel;
    }

    // ---------------------------------------------
    // @Note: gather paths.
    {
        Utf8 binary_path = {};
        Utf8 appdata_path = {};
        {
            Temporary_Arena tmp = temporary_arena_begin(os->arena);

            {
                u64 size = KB(32);
                u16 *buffer = push_array_noz(tmp.arena, u16, size);
                DWORD length = GetModuleFileNameW(0, (WCHAR *)buffer, size);
                binary_path = to_utf8(tmp.arena, utf16(buffer, length));
                binary_path = utf8_path_chop_last_slash(binary_path);
            }

            {
                u64 size = KB(32);
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

    // ---------------------------------------------
    // @Note: init key table.
    for (u32 i = 'A', j = OS_KEY_A; i <= 'Z'; i += 1, j += 1)
    {
        os->key_table[i] = (Os_Key)j;
    }
    for (u32 i = '0', j = OS_KEY_0; i <= '9'; i += 1, j += 1)
    {
        os->key_table[i] = (Os_Key)j;
    }
    for (u32 i = VK_F1, j = OS_KEY_F1; i <= VK_F24; i += 1, j += 1)
    {
        os->key_table[i] = (Os_Key)j;
    }
    os->key_table[VK_LBUTTON]       = OS_KEY_MOUSE_LEFT;
    os->key_table[VK_RBUTTON]       = OS_KEY_MOUSE_RIGHT;
    os->key_table[VK_MBUTTON]       = OS_KEY_MOUSE_MIDDLE;
    os->key_table[VK_ESCAPE]        = OS_KEY_ESC;
    os->key_table[VK_OEM_3]         = OS_KEY_GRAVE_ACCENT;
    os->key_table[VK_OEM_MINUS]     = OS_KEY_MINUS;
    os->key_table[VK_OEM_PLUS]      = OS_KEY_EQUAL;
    os->key_table[VK_BACK]          = OS_KEY_BACKSPACE;
    os->key_table[VK_TAB]           = OS_KEY_TAB;
    os->key_table[VK_SPACE]         = OS_KEY_SPACE;
    os->key_table[VK_RETURN]        = OS_KEY_ENTER;
    os->key_table[VK_CONTROL]       = OS_KEY_CTRL;
    os->key_table[VK_SHIFT]         = OS_KEY_SHIFT;
    os->key_table[VK_MENU]          = OS_KEY_ALT;
    os->key_table[VK_UP]            = OS_KEY_UP;
    os->key_table[VK_LEFT]          = OS_KEY_LEFT;
    os->key_table[VK_DOWN]          = OS_KEY_DOWN;
    os->key_table[VK_RIGHT]         = OS_KEY_RIGHT;
    os->key_table[VK_DELETE]        = OS_KEY_DELETE;
    os->key_table[VK_PRIOR]         = OS_KEY_PAGE_UP;
    os->key_table[VK_NEXT]          = OS_KEY_PAGE_DOWN;
    os->key_table[VK_HOME]          = OS_KEY_HOME;
    os->key_table[VK_END]           = OS_KEY_END;
    os->key_table[VK_OEM_2]         = OS_KEY_FORWARD_SLASH;
    os->key_table[VK_OEM_PERIOD]    = OS_KEY_PERIOD;
    os->key_table[VK_OEM_COMMA]     = OS_KEY_COMMA;
    os->key_table[VK_OEM_7]         = OS_KEY_QUOTE;
    os->key_table[VK_OEM_4]         = OS_KEY_LEFT_BRACKET;
    os->key_table[VK_OEM_6]         = OS_KEY_RIGHT_BRACKET;
    os->key_table[VK_INSERT]        = OS_KEY_INSERT;
    os->key_table[VK_OEM_1]         = OS_KEY_SEMICOLON;
}
