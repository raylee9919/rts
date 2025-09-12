#ifndef RTS_BASE_THREAD_CTX_H
#define RTS_BASE_THREAD_CTX_H
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Seong Woo Lee $
   $Notice: (C) Copyright %s by Seong Woo Lee. All Rights Reserved. $
   ======================================================================== */


struct Arena;

struct Thread_Context
{
    Arena *scratch_arena;
};

per_thread Thread_Context tctx;

internal void thread_init(void);


#endif // RTS_BASE_THREAD_CTX_H
