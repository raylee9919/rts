/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Seong Woo Lee $
   $Notice: (C) Copyright %s by Seong Woo Lee. All Rights Reserved. $
   ======================================================================== */

internal void
thread_init(void)
{
    tctx.scratch_arena = arena_alloc();
}

internal Temporary_Arena
scratch_begin(void)
{
    Temporary_Arena scratch = temporary_arena_begin(tctx.scratch_arena);
    return scratch;
}

internal void
scratch_end(Temporary_Arena scratch)
{
    temporary_arena_end(scratch);
}
