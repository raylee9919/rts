// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RTS_ASSET_H
#define RTS_ASSET_H

#include "basic/allocator.h"
#include "basic/hash_table.h"
#include "os/os.h"

enum Asset_Kind : u16 {
    ASSET_MESH,
    ASSET_MATERIAL,
    ASSET_KIND_COUNT
};

force_inline u32 asset_guid_to_u32(Guid guid) {
    return guid._32[0] ^ guid._32[1] ^ guid._32[2] ^ guid._32[3];
}

struct Asset_Entry {
    s64 ref_count;
};

struct Asset_System {
    Allocator heap;

    Table<Guid, Asset_Entry, asset_guid_to_u32> asset_table;
};

extern Asset_System *asset_system;

void asset_system_init();
void asset_system_shutdown();


#endif // RTS_ASSET_H
