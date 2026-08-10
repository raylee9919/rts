// Copyright Seong Woo Lee. All Rights Reserved.

#include "basic/include.h"
#include "math/include.h"
#include "os/include.h"

#include "basic/include.cpp"
#include "math/include.cpp"
#include "os/include.cpp"

void work(void *param) {
    printf("%llu\n", (u64)param);
}

int main_entry(int argc, char **argv)
{
    Thread_Group *group = NULL;
    {
        Arena *arena = arena_alloc();
        group = push_struct(arena, Thread_Group);
        thread_group_init(group, 8, arena, S("WorkerThreadGroup"));
    }

    s32 *values = new s32[1000];

    for (int i = 0; i < 1000; ++i) {
        values[i] = 0;
    }

    parallel_for(group, 1000, [&](s64 i) {
        values[i] += i;
    });


    thread_group_shutdown(group);

    return 0;
}
