// Copyright Seong Woo Lee. All Rights Reserved.

void context_push(Context context) {
    auto *ts = &thread_state;
    Assert(ts->context_stack_pointer < (s64)array_count(ts->context_stack));
    tctx = ts->context_stack[++ts->context_stack_pointer] = context;
}

void context_pop() {
    auto *ts = &thread_state;
    Assert(ts->context_stack_pointer > 0);
    tctx = ts->context_stack[--ts->context_stack_pointer];
}

// @Todo: Roll my own heap.
void *crt_proc(Allocator_Mode mode, u64 size, u64 old_size, void *old_memory, void *data) {
    void *result = NULL;

    if (mode == ALLOCATOR_MODE_ALLOCATE) {
        result = malloc(size);
        memset(result, 0, size);
    } else if (mode == ALLOCATOR_MODE_RESIZE) {
        result = realloc(old_memory, size);
        if (size > old_size) {
            memset((u8 *)result + old_size, 0, size - old_size);
        }
    } else if (mode == ALLOCATOR_MODE_FREE) {
        free(old_memory);
    } else {
        Assert(!"X");
    }

    return result;
}

void thread_init() {
    tctx.scratch_arena = arena_alloc();

    // @Temporary
    {
        tctx.allocator.proc = crt_proc;
        tctx.allocator.data = NULL;

        tctx.temp.proc = arena_allocator_proc;
        tctx.temp.data = arena_alloc();
    }

    context_push(tctx);
}

Temporary_Arena scratch_begin(void) {
    Temporary_Arena scratch = temporary_arena_begin(tctx.scratch_arena);
    return scratch;
}

void scratch_end(Temporary_Arena scratch) {
    temporary_arena_end(scratch);
}

void clear_temporary_storage() {
    tctx.temp.proc(ALLOCATOR_MODE_FREE, 0, 0, NULL, &tctx.temp);
}
