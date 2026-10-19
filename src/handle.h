// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RTS_HANDLE_H
#define RTS_HANDLE_H

#include "basic/core.h"


// You can think of Handle as a pointer. The difference is that while a
// "real" pointers is in the process's address space, a Handle is an offset
// into the Game_Storage's HUGE reserved address space.
//
struct Handle {
    u64             offset;
    u64             generational_id;

    force_inline bool operator == (Handle &other) { return ((offset == other.offset) && (generational_id == other.generational_id)); }
    force_inline bool operator != (Handle &other) { return ((offset != other.offset) || (generational_id != other.generational_id)); }
    force_inline bool is_null() { return ((offset == 0) && (generational_id == 0)); }
};


#endif // RTS_HANDLE_H
