

#if 0
PLATFORM_WORK_QUEUE_CALLBACK(load_asset_work)
{
    Load_Asset_Work_Data *workData = (Load_Asset_Work_Data *)data;
    workData->game_assets->bitmaps[workData->assetID] = load_bmp(workData->assetArena, workData->game_assets->read_entire_file, workData->fileName);
    workData->game_assets->bitmapStates[workData->assetID] = Asset_State_Loaded;
    end_work_memory(workData->workSlot);
}
#endif

#if 0
internal Bitmap *
GetBitmap(Transient_State *trans_state, Asset_ID assetID,
          PlatformWorkQueue *queue, Platform_Api *platform)
{
    Bitmap *result = trans_state->game_assets.bitmaps[assetID];

    if (!result) 
    {
        if (atomic_compare_exchange_u32((u32 *)&trans_state->game_assets.bitmapStates[assetID],
                                        Asset_State_Queued, Asset_State_Unloaded)) 
        {
            Work_Memory_Arena *workSlot = begin_work_memory(trans_state);
            if (workSlot) 
            {
                Load_Asset_Work_Data *workData = push_struct(&workSlot->memory_arena, Load_Asset_Work_Data);
                workData->game_assets = &trans_state->game_assets;
                workData->assetArena = &trans_state->assetArena;
                workData->assetID = assetID;
                workData->workSlot = workSlot;

                switch(assetID) 
                {
                    case GAI_Tree: 
                    {
                        workData->fileName = "tree2_teal.bmp";
                    } break;

                    case GAI_Particle: 
                    {
                        workData->fileName = "white_particle.bmp";
                    } break;

                    INVALID_DEFAULT_CASE
                }
#if 1 // multi-thread
                platform->platform_add_entry(queue, load_asset_work, workData);
                return 0; // todo: no bmp...?
#else // single-thread
                load_asset_work(queue, workData);
                return 0; // todo: no bmp...?
#endif
            } 
            else 
            {
                return result;
            }
        } 
        else 
        {
            return result;
        }
    } 
    else 
    {
        return result;
    }
}
#endif
