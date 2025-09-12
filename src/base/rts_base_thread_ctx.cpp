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
    tctx.scratch_arena = arena_alloc(KB(256));
}
