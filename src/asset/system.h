// Copyright Seong Woo Lee. All Rights Reserved.

#pragma once

namespace Asset
{
    struct System
    {
        u64 next_incremental_id;
    };

    void init(System *sys);
}
