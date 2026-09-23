// Copyright Seong Woo Lee. All Rights Reserved.

#include "asset/asset.h"
#include "basic/context.h"
#include "basic/log.h"
#include "shared.h"
#include "material/material.h"
#include "asset/mesh.h"

Asset_System *asset_system;

static b32 asset_load( Guid id );

void asset_system_init() 
{
    Allocator heap = { crt_proc, nullptr };
    asset_system = (Asset_System *)alloc(sizeof(Asset_System), heap);
    Construct(asset_system);
    asset_system->heap = heap;

    asset_system->asset_table.allocator        = heap;
    asset_system->guid_to_short_name.allocator = heap;
    asset_system->type_infos.allocator         = heap;

    {
        Asset_Type_Info info = {};
        info.path_extension   = S("material");
        info.load_proc        = material_load_proc;
        asset_type_register(info);
    }
    {
        Asset_Type_Info info = {};
        info.path_extension   = S("png");
        info.load_proc        = image_load_proc;
        asset_type_register(info);
    }
    {
        Asset_Type_Info info = {};
        info.path_extension   = S("texture");
        info.load_proc        = texture_load_proc;
        asset_type_register(info);
    }
    {
        Asset_Type_Info info = {};
        info.path_extension   = S("triangle_mesh");
        info.load_proc        = Asset::mesh_load_proc;
        asset_type_register(info);
    }

    asset_system_init_catalog();
}

Array<String> asset_file_list(String path, 
                              Allocator allocator, 
                              b32 follow_directory_symlinks)
{
    Array<String> files = {};
    files.allocator = allocator;

    auto visitor = [](File_Visit_Info *info, void *user_data) {
        auto *arr = (Array<String>*)user_data;
        array_add(arr, copy_string(asset_shortname(info->full_name), arr->allocator));
    };

    visit_files(path, true, &files, visitor, follow_directory_symlinks);

    return files;
}

void asset_system_init_catalog()
{
    // Catalog
    // Strings are allocated in the heap at the moment. 
    // So when you free the entry from the table, you must 
    // free the according string memory as well.
    // @Robustness
    Array<String> fl = asset_file_list(shared->data_path, asset_system->heap, true);
    for (int i = 0; i < fl.count; ++i) 
    {
        String short_name = fl.data[i];
        String path = tprint(S("%S/%S"), shared->data_path, short_name);
        auto [ext, ext_success] = path_extension(path);
        if ( ext_success ) 
        {
            // If from short name, not absoule path!
            Guid id = guid_from_string(short_name);

            // If the extension matches one of the registered asset type, 
            // mapping to the filepath from Guid is added.
            for ( Asset_Type_Info& info : asset_system->type_infos )
            {
                if ( info.path_extension && (info.path_extension == ext) )
                {
                    table_add(&asset_system->guid_to_short_name, id, short_name);
                    break;
                }
            }
        }
    }
}

void asset_system_shutdown() 
{
    release(asset_system->heap);
    memset(asset_system, 0, sizeof(Asset_System));
}

void asset_type_register( Asset_Type_Info info )
{
    if ( info.path_extension )
    {
        if (!array_add_unique(&asset_system->type_infos, info)) {
            log_warning(S("asset type with path extension: '%S' was already registered."), info.path_extension);
        } else {
            log_info(S("Registerd asset type with path extension: '%S'"), info.path_extension);
        }
    }
    else
    {
        log_error(S("asset type to be registered must have path extension."));
    }
}

void asset_request( Guid id ) 
{
    auto *entry = table_find_pointer(&asset_system->asset_table, id);
    if ( !entry )
    {
        Asset_Entry e = {};
        entry = table_add(&asset_system->asset_table, id, e);
    }

    R_ASSERT( entry );
    if ( entry->ref_count == 0 )
    {
        asset_load(id); 
    }
    entry->ref_count += 1;
}

void asset_drop( Guid id ) 
{
    auto *entry = table_find_pointer(&asset_system->asset_table, id);
    R_ASSERT( entry );

    entry->ref_count -= 1;

    if ( entry->ref_count == 0 ) {
        // @Todo: unlaod asset
        table_remove(&asset_system->asset_table, id);
    }
}

bool assest_type_info_cmp(Asset_Type_Info a, Asset_Type_Info b)
{
    if (a.path_extension == b.path_extension) return true;
    return false;
}

String asset_shortname( String path )
{
    String short_name = path;

    R_ASSERT(begins_with(path, shared->data_path));

    advance(&short_name, shared->data_path.len);
    short_name = trim_left(short_name, S("./\\"));

    return short_name;
}

Guid asset_id_from_path( String path )
{
    String s = asset_shortname(path);
    return guid_from_string(s);
}

static b32 asset_load( Guid id )
{
    // Entry must have been added if not exist during request.
    auto *entry = table_find_pointer(&asset_system->asset_table, id);
    R_ASSERT( entry );

    // Find path from the catalog.
    String *short_name = table_find_pointer(&asset_system->guid_to_short_name, id);
    if ( !short_name )
    {
        log_error(S("Asset with short-name:'%S', GUID: '%llu-%llu' couldn't be found."), *short_name, id._64[1], id._64[0]);
        return false;
    }

    // Check extension and dispatch according routine.
    // This might turn into callback function later on. idk.
    // @Todo
    String path = tprint(S("%S/%S"), shared->data_path, *short_name);
    auto [ext, ext_ok] = path_extension(path);
    if ( !ext_ok ) {
        log_error(S("Failed to acquire file extension from: '%S'"), path);
        return false;
    }

    bool ext_match = false;
    for ( Asset_Type_Info& info : asset_system->type_infos )
    {
        if ( info.path_extension && (info.path_extension == ext) )
        {
            R_ASSERT(info.load_proc);
            info.load_proc(path, *short_name, nullptr);
            ext_match = true;
            break;
        }
    }

    if ( !ext_match )
    {
        log_error(S("Unregistered file extension '%S', path: '%S'."), ext, path);
        return false;
    }

    return true;
}
