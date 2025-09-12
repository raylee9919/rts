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
// @Note: Memory
internal
OS_QUERY_PAGE_SIZE(win32_query_page_size)
{
    SYSTEM_INFO info;
    GetSystemInfo(&info);
    return info.dwPageSize;
}

internal
OS_RESERVE(win32_memory_reserve)
{
    DWORD alloc_flags   = MEM_RESERVE;
    DWORD protect_flags = PAGE_NOACCESS;
    if (commit)
    {
        alloc_flags   |= MEM_COMMIT;
        protect_flags = PAGE_READWRITE;
    }

    void *result = VirtualAlloc(0, size, alloc_flags, protect_flags);
    return result;
}

internal
OS_RELEASE(win32_memory_release)
{
    VirtualFree(ptr, 0, MEM_RELEASE);
}

internal
OS_COMMIT(win32_memory_commit)
{
    mmm page_size = win32_query_page_size();
    mmm page_snapped_size = size;
    page_snapped_size +=  page_size - 1;
    page_snapped_size -= page_snapped_size % page_size;
    VirtualAlloc(ptr, page_snapped_size, MEM_COMMIT, PAGE_READWRITE);
}

internal
OS_DECOMMIT(win32_memory_decommit)
{
    VirtualFree(ptr, size, MEM_DECOMMIT);
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
    mmm result = 0;

    HANDLE handle = to_win32_handle(file);

    LARGE_INTEGER off_li = {};
    off_li.QuadPart = 0;
    if (handle == INVALID_HANDLE_VALUE)
    {
        Assert(0);
    }
    else if (SetFilePointerEx(handle, off_li, 0, FILE_BEGIN))
    {
        mmm bytes_to_read = size;
        mmm bytes_actually_read = 0;

        u8 *ptr = (u8 *)dst;
        u8 *opl = ptr + bytes_to_read;

        for(;;)
        {
            mmm unread = (mmm)(opl - ptr);
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
// @Note: Init
internal
OS_INIT(os_win32_init)
{
    os.file_is_valid  = win32_file_is_valid;
    os.file_open      = win32_file_open;
    os.file_close     = win32_file_close;
    os.file_size      = win32_file_size;
    os.file_read      = win32_file_read;
    os.file_delete    = win32_file_delete;
    os.file_move      = win32_file_move;
    os.file_copy      = win32_file_copy;
    os.make_directory = win32_make_directory;

    os.query_page_size = win32_query_page_size;
    os.memory_reserve  = win32_memory_reserve;
    os.memory_release  = win32_memory_release;
    os.memory_commit   = win32_memory_commit;
    os.memory_decommit = win32_memory_decommit;

    os.abort = win32_abort;
}
