// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RTS_BASE_CONTEXT_H
#define RTS_BASE_CONTEXT_H


struct Context {
    struct Arena *scratch_arena;

    struct Arena *temporary_arena;

    Allocator allocator;

    Allocator temp;
};
per_thread Context tctx;


struct Thread_State {
    Context context_stack[32]     = {};
    s64     context_stack_pointer = -1;
};
per_thread Thread_State thread_state;


void    context_push(Context context);
void    context_pop();
#define push_context(CTX) defer_loop(context_push(CTX), context_pop())




internal void thread_init(void);

internal Temporary_Arena scratch_begin(void);
internal void            scratch_end(Temporary_Arena tmp);

internal void clear_temporary_storage();



#endif // RTS_BASE_CONTEXT_H
