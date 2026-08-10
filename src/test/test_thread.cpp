// Copyright Seong Woo Lee. All Rights Reserved.

#include "basic/include.h"
#include "math/include.h"
#include "os/include.h"

#include "basic/include.cpp"
#include "math/include.cpp"
#include "os/include.cpp"

#define WORK_COUNT 1024

int main_entry(int argc, char **argv)
{
    Thread_Group *group = NULL;
    {
        Arena *arena = arena_alloc();
        group = push_struct(arena, Thread_Group);
        thread_group_init(group, 8, arena, S("WorkerThreadGroup"));
    }

    s32 *values = new s32[WORK_COUNT];

    for (int i = 0; i < WORK_COUNT; ++i) {
        values[i] = 0;
    }

    parallel_for(group, WORK_COUNT, [&](s64 i) {
        values[i] += i;
    });


    thread_group_shutdown(group);

    return 0;
}
