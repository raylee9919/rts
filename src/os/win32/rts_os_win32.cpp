/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Seong Woo Lee $
   $Notice: (C) Copyright %s by Seong Woo Lee. All Rights Reserved. $
   ======================================================================== */



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
OS_FILE_COPY(win32_file_copy)
{
    Temporary_Arena scratch = scratch_begin();
    {
        Utf16 src16 = to_utf16(scratch.arena, src);
        Utf16 dst16 = to_utf16(scratch.arena, dst);
        CopyFileW((WCHAR *)src16.str, (WCHAR *)dst16.str, FALSE);
    }
    scratch_end(scratch);
}

// --------------------------------------
// @Note: Init
internal
OS_INIT(os_win32_init)
{
    os.file_copy = win32_file_copy;

    os.query_page_size = win32_query_page_size;
    os.memory_reserve  = win32_memory_reserve;
    os.memory_release  = win32_memory_release;
    os.memory_commit   = win32_memory_commit;
    os.memory_decommit = win32_memory_decommit;
}
