// Copyright Seong Woo Lee. All Rights Reserved.

#include "base/rts_base_inc.h"
#include "os/os.h"

#include "base/rts_base_inc.cpp"
#include "os/os.cpp"

struct Foo {
    Foo *next;
    Foo *prev;
    int x;
};

int main_entry(int argc, char** argv)
{
    // Init
    os_init();
    thread_init();

    // Read file test.
    if (0) {
        Utf8 path = utf8lit("C:/Users/swl/Desktop/File-5GB.dat");
        OS_Handle file = os_open_file(path, OS_ACCESS_FLAG_READ);
        u64 size = os_get_file_size(file);
        u8* ptr = new u8[size];
        os_read_file(file, 0, size, ptr);
        os_close_file(file);
    }

    // 'list_for' test.
    if (0) {
        Foo* first_foo = NULL;
        Foo* last_foo  = NULL;

        for (int i = 0; i < 31; ++i) {
            Foo* foo = new Foo;
            foo->x = i;
            dll_push_back(first_foo, last_foo, foo);
        }

        list_for(first_foo, it) {
            if (it->x % 3 == 0) {
                dll_remove(first_foo, last_foo, it);
                delete it;
                continue;
            }
            printf("%d\n", it->x);
        }
    }

    return 0;
}
