// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RTS_BASE_CONTEXT_H
#define RTS_BASE_CONTEXT_H

#include "basic/core.h"
#include "basic/arena.h"
#include "basic/allocator.h"

struct Context {
    Arena *scratch_arena;

    Arena *temporary_arena;

    Allocator allocator;

    Allocator temp;
};
extern per_thread Context tctx;


struct Thread_State {
    Context context_stack[32]     = {};
    s64     context_stack_pointer = -1;
};
extern per_thread Thread_State thread_state;


void    context_push(Context context);
void    context_pop();
#define push_context(CTX) defer_loop(context_push(CTX), context_pop())




void thread_init();

Temporary_Arena scratch_begin(void);
void            scratch_end(Temporary_Arena tmp);

void clear_thread_temporary_storage();



#endif // RTS_BASE_CONTEXT_H
