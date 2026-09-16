// Copyright Seong Woo Lee. All Rights Reserved.

#include "asset/asset.h"
#include "basic/context.h"

Asset_System *asset_system;

void asset_system_init() {
    Allocator heap = { crt_proc, nullptr };
    asset_system = (Asset_System *)alloc(sizeof(Asset_System), heap);
    Construct(asset_system);
    asset_system->heap = heap;

    asset_system->asset_table.allocator = heap;
}

void asset_system_shutdown() {
    release(asset_system->heap);
    memset(asset_system, 0, sizeof(Asset_System));
}

#if 0
static void add_ref(Guid guid) {
    Asset_Entry *asset = table_find_pointer(&asset_system->asset_table, guid);
    if (!asset) {
        asset = table_add(&asset_system->asset_table, guid, {});
    }

    asset->ref_count += 1;

    if (asset->ref_count == 1) {
        // @Todo(swL): load asset
    }
}

Asset_ID::~Asset_ID() {
    Asset_Entry *asset = table_find_pointer(&asset_system->asset_table, guid);
    Assert(asset && asset->ref_count != 0);

    asset->ref_count -= 1;

    if (asset->ref_count == 0) {
        // @Todo(swL): unload asset
    }

    printf("sub ref\n");
}
#endif
