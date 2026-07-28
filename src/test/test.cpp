// Copyright Seong Woo Lee. All Rights Reserved.

#include "Basic/include.h"
#include "math/math.h"
#include "os/os.h"

#include "Basic/include.cpp"
#include "math/math.cpp"
#include "os/os.cpp"

static void test_array();
static void test_table();

int main_entry(int argc, char **argv)
{
    test_array();
    test_table();

    return 0;
}

static void test_array()
{
    Array<int> arr = {};

    array_add(&arr, 1);
    array_add(&arr, 2);
    array_add(&arr, 3);
    array_add(&arr, 4);
    array_add(&arr, 5);
    array_add(&arr, 6);
    array_add(&arr, 7);
    array_add(&arr, 8);
    array_add(&arr, 9);

    for (int x : arr) {
        printf("%d\n", x);
    }

    array_reset(&arr);

    for (int x : arr) {
        printf("%d\n", x);
    }
}

static void test_table()
{
    Table<int, int> t = {};

    {
        for (int i = 0; i < 44; ++i)  table_add(&t, i, i + 1);
        for (int i = 0; i < 30; ++i)  table_remove(&t, i);
        table_add(&t, 44, 45);
    }

    {
        for (int i = 0; i < 30; ++i) {
            auto find_result = table_find(&t, i);
            Assert(!find_result.found);
        }

        for (int i = 30; i <= 44; ++i) {
            auto find_result = table_find(&t, i);
            Assert(find_result.found);
        }
    }

    table_reset_keeping_memory(&t);

    table_add(&t, 6, 23);

    table_reset(&t);
}
