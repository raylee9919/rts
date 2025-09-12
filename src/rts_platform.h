#ifndef RTS_PLATFORM_H
#define RTS_PLATFORM_H
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Seong Woo Lee $
   $Notice: (C) Copyright %s by Seong Woo Lee. All Rights Reserved. $
   ======================================================================== */

struct Platform_Game_Memory 
{
    OS os;
    b32 executable_reloaded;

    Arena *arena;
    void *game_state;
};

#define GAME_UPDATE_AND_RENDER(name) void name(struct Platform_Game_Memory *game_memory,\
                                               struct Input *input,\
                                               struct Event_Queue *event_queue,\
                                               struct Render_Commands *render_commands)
typedef GAME_UPDATE_AND_RENDER(Game_Update_And_Render);


#endif //RTS_PLATFORM_H
