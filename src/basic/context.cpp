// Copyright Seong Woo Lee. All Rights Reserved.

void context_push(Context context) {
    auto* ts = &thread_state;
    assert(ts->context_stack_pointer < (s64)array_count(ts->context_stack));
    tctx = ts->context_stack[++ts->context_stack_pointer] = context;
}

void context_pop() {
    auto* ts = &thread_state;
    assert(ts->context_stack_pointer > 0);
    tctx = ts->context_stack[--ts->context_stack_pointer];
}


// @Temporary
// @Temporary
// @Temporary
// @Temporary
void* arena_proc(Allocator_Mode mode, u64 size, u64 old_size, void* old_memory, void* data) {
    auto* arena = (Arena*)data;

    if (mode == ALLOCATOR_MODE_ALLOCATE) {
        return push_size(arena, size);
    } else if (mode == ALLOCATOR_MODE_FREE) {
        arena_clear(arena);
        return nullptr;
    } else {
        assert(!"X");
        return nullptr;
    }
}
// @Temporary
// @Temporary
// @Temporary
// @Temporary


void thread_init(void) {
    tctx.scratch_arena = arena_alloc();

    // @Temporary
    // @Temporary
    // @Temporary
    // @Temporary
    tctx.temp.proc = arena_proc;
    tctx.temp.data = arena_alloc();
    // @Temporary
    // @Temporary
    // @Temporary
    // @Temporary

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
    tctx.temp.proc(ALLOCATOR_MODE_FREE, 0, 0, nullptr, &tctx.temp);
}
