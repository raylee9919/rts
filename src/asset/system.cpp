// Copyright Seong Woo Lee. All Rights Reserved.

#include "asset/system.h"

namespace Asset
{
    void init(System *sys)
    {
        sys->next_incremental_id = 1;
    }
}


void asset_system_init(Asset_System *sys) {
    sys->next_incremental_id = 1;
}
