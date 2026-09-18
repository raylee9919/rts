// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RTS_ASSET_H
#define RTS_ASSET_H

#include "basic/allocator.h"
#include "basic/hash_table.h"
#include "os/os.h"

force_inline u32 asset_guid_to_u32(Guid guid) {
    return guid._32[0] ^ guid._32[1] ^ guid._32[2] ^ guid._32[3];
}

struct Asset_Entry {
    s64 ref_count;
};

struct Asset {
    u64  type_hash;
    Guid guid;
};

struct Asset_Type_Info {
    String  path_extension;

    void (*load_proc)(String filepath, String short_name, void *user_data);

    bool operator == (Asset_Type_Info &other) {
        return path_extension == other.path_extension;
    }
};

struct Asset_System {
    Allocator heap;

    Table<Guid, Asset_Entry, asset_guid_to_u32> asset_table;
    Table<Guid, String, asset_guid_to_u32> guid_to_short_name;

    Array<Asset_Type_Info> type_infos;
};

extern Asset_System *asset_system;

void asset_system_init();
void asset_system_shutdown();
void asset_system_init_catalog();

void asset_type_register( Asset_Type_Info info );

void asset_request( Guid id );
void asset_drop( Guid id );

bool assest_type_info_cmp(Asset_Type_Info a, Asset_Type_Info b);

String asset_shortname( String path );
Guid   asset_id_from_path( String path );


#endif // RTS_ASSET_H
