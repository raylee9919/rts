/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Sung Woo Lee $
   $Notice: (C) Copyright %s by Sung Woo Lee. All Rights Reserved. $
   ======================================================================== */

#include "animation.h"
#include "random.h"

struct Camera;
struct Ui;
struct Debug_State;
struct Entity;

#define MAX_ENTITY_COUNT 1024

struct Animation_Channel {
    Animation *animation;
    f32 dt;
};

struct World {
    Entity *entities[MAX_ENTITY_COUNT];
    u32 entity_count;

    Camera *cameras[256];
    u32 camera_count;

    u32 next_entity_id;
};

struct Kerning {
    u32         first;
    u32         second;
    s32         value;
    Kerning     *prev;
    Kerning     *next;
};
struct Kerning_List {
    Kerning     *first;
    Kerning     *last;
    u32         count;
};
struct Kerning_Hashmap {
    Kerning_List entries[1024];
};

internal  u32
kerning_hash(Kerning_Hashmap *hashmap, u32 first, u32 second) 
{
    // @TODO: better hash function.
    u32 result = (first * 12 + second * 33) % arraycount(hashmap->entries);
    return result;
}

internal void
push_kerning(Kerning_Hashmap *hashmap, Kerning *kern, u32 entry_idx) 
{
    Assert(entry_idx < arraycount(hashmap->entries));
    Kerning_List *list = hashmap->entries + entry_idx;
    if (list->first) {
        list->last->next = kern;
        kern->prev = list->last;
        kern->next = 0;
        list->last = kern;
        ++list->count;
    } else {
        list->first = kern;
        list->last = kern;
        kern->prev = 0;
        kern->next = 0;
        ++list->count;
    }
}

internal s32
get_kerning(Kerning_Hashmap *hashmap, u32 first, u32 second) 
{
    s32 result = 0;
    u32 entry_idx = kerning_hash(hashmap, first, second);
    Assert(entry_idx < arraycount(hashmap->entries));
    for (Kerning *at = hashmap->entries[entry_idx].first; at; at = at->next) {
        if (at->first == first && at->second == second) {
            result = at->value;
        }
    }
    return result;
}

struct Asset_Font {
    u32             v_advance;
    f32             ascent;
    f32             descent;
    f32             max_width;
    Kerning_Hashmap kern_hashmap;
    Asset_Glyph     *glyphs[256];
    Kerning kernings[4096];

    // @Temporary
    Buffer buffer;
    u64 writetime;
};

struct Game_Assets {
    Bitmap debug_bitmap;

    Asset_Font debug_font;
    Asset_Font menu_font;
    Asset_Font console_font;
    Asset_Font karmina;
    Asset_Font times;

    Model *xbot_model;
    Model *crate_model;

    Mesh skybox_mesh;
    Bitmap skybox_textures[6];

    Model *sphere_model;
    Model *plane_model;

    Model *rock_model;

    Animation *xbot_idle;
    Animation *xbot_run;
    Animation *xbot_die;
    Animation *xbot_attack;
};

enum Game_Mode {
    Game_Mode_Editor,
    Game_Mode_Game,
    Game_Mode_Menu,
};

struct Game_State {
    b32 initted;

    b32 editor_initted;

    Memory_Arena total_arena;

    Memory_Arena frame_arena;
    Temporary_Memory frame_temporary_memory;

    Memory_Arena asset_arena;
    Game_Assets *game_assets;

    Memory_Arena world_arena;
    World *world;

    Memory_Arena debug_arena;
    Debug_State *debug_state;

    Memory_Arena ui_arena;

    f32 real_time;
    f32 game_time;

    Game_Mode mode;

    Random_Series random_series;

    Camera *controlling_camera;
    m4x4 view_proj; // @Ain't thrilled about it. -Ray
    Camera *game_camera;
    Camera *debug_camera;
    Camera *orthographic_camera;

    u32 active_entity_id;

    Platform_Work_Queue *high_priority_queue;
    Platform_Work_Queue *low_priority_queue;
    Work_Memory_Arena work_arenas[4];

    b32 explorer;
    char explorerpath[256];
    Platform_File_List filelist;
    b32 filelist_changed;

    Navmesh navmesh;
};

struct Load_Asset_Work_Data {
    Game_Assets         *game_assets;
    Memory_Arena        *assetArena;
    Asset_ID            assetID;
    const char          *fileName;
    Work_Memory_Arena   *workSlot;
};
