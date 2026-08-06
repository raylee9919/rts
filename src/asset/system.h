// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RTS_ASSET_SYSTEM_H
#define RTS_ASSET_SYSTEM_H

namespace Asset
{
    struct System
    {
        u64 next_incremental_id;
    };

    void init(System *sys);
}


// @Todo: Deprecate upper part.

struct Asset_System {
    u64         next_incremental_id;
};

internal void asset_system_init(Asset_System *sys);


#endif // RTS_ASSET_SYSTEM_H
