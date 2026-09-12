// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RTS_ARENA_H
#define RTS_ARENA_H

#include "basic/core.h"
#include "basic/allocator.h"


#define ARENA_DEFAULT_RESERVE_SIZE  (64ull * 1024 * 1024)
#define ARENA_DEFAULT_COMMIT_SIZE   (64ull * 1024 * 1024)
#define ARENA_HEADER_SIZE           (128ull)


struct Arena {
    Arena* prev;
    Arena* current;

    u64 cmt_size;
    u64 rsv_size;
    u64 base_pos;
    u64 pos;
    u64 cmt;
    u64 rsv;
};
static_assert(ARENA_HEADER_SIZE >= sizeof(Arena));

struct Temporary_Arena {
    Arena* arena;
    u64 pos;
};


Arena* arena_alloc_(u64 rsv_size, u64 cmt_size);
#define arena_alloc() arena_alloc_(ARENA_DEFAULT_RESERVE_SIZE, ARENA_DEFAULT_COMMIT_SIZE)
void arena_release(Arena *arena);

void* arena_push(Arena *arena, u64 size, u64 align);

u64 arena_pos(Arena *arena);
void arena_pop_to(Arena *arena, u64 pos);
void arena_clear(Arena *arena);
void arena_pop(Arena *arena, u64 size);

#define push_array_noz_aligned(a, T, n, align)  (T *)arena_push((a), sizeof(T)*(n), (u64)(align))
#define push_array_aligned(a, T, n, align)      (T *)memset(push_array_noz_aligned(a, T, n, (u64)align), 0, sizeof(T)*(n))
#define push_array_noz(a, T, n)                      push_array_noz_aligned(a, T, n, max(8ull, (u64)align_of(T)))
#define push_array(a, T, n)                          push_array_aligned(a, T, n, max(8ull, (u64)align_of(T)))

#define push_struct_noz(a, T)                        push_array_noz(a, T, 1)
#define push_struct(a, T)                            push_array(a, T, 1)

#define push_size(a, s)                              arena_push(a, s, 8)


Temporary_Arena    temporary_arena_begin(Arena* arena);
void               temporary_arena_end(Temporary_Arena temp);
//Temporary_Arena    scratch_begin(void);
//void               scratch_end(Temporary_Arena tmp);


void              *arena_allocator_proc(Allocator_Mode mode, u64 size, u64 old_size, void *old_memory, void *data);
Allocator          arena_allocator_alloc();


#endif // RTS_ARENA_H
