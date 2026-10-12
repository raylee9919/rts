// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RTS_ASSET_H
#define RTS_ASSET_H

#include "basic/allocator.h"
#include "basic/hash_table.h"
#include "os/os.h"


#define ASSET_LOAD_PROC(Name) void Name(Guid id, Guid referrer, void *user_data)
typedef ASSET_LOAD_PROC(Asset_Load_Proc);


struct Asset_Entry 
{
  s64 ref_count;
};

struct Asset_Type_Info 
{
  String path_extension;

  Asset_Load_Proc *load_proc;

  bool operator == (Asset_Type_Info &other) {
    return path_extension == other.path_extension;
  }
};

struct Asset_System 
{
  Allocator heap;

  Table<Guid, Asset_Entry, hash_guid> asset_table;
  Table<Guid, String, hash_guid> guid_to_path;

  Array<Asset_Type_Info> type_infos;
};

extern Asset_System *asset_system;


String
asset_name_from_guid(Guid id);

void 
asset_system_init();

void 
asset_system_shutdown();

void 
asset_type_register(Asset_Type_Info info);

void 
asset_request(Guid id, Guid referrer);

void 
asset_drop(Guid id);

bool 
asset_type_info_cmp(Asset_Type_Info a, 
                    Asset_Type_Info b);


#endif // RTS_ASSET_H
