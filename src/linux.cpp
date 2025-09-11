/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Sung Woo Lee $
   $Notice: (C) Copyright %s by Sung Woo Lee. All Rights Reserved. $
   ======================================================================== */


#include <sys/mman.h>

// @NOTE: X11 Windowing.
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>

#include <math.h>

#include "types.h"
#include "game.h"
#include "linux.h"


internal void
linux_init_render_batch(Render_Batch *batch, size_t size) 
{
    batch->base   = mmap(0, size, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
    batch->size   = size;
    batch->used   = 0;

    batch->width  = 0;
    batch->height = 0;
}

int main(int argc, char **argv)
{
    char *default_display = getenv("DISPLAY");
    Display *display = 0;
    if (default_display) {
        display = XOpenDisplay(default_display);
    }

    if (!display) {
        display = XOpenDisplay(0);
    }

    if (display) {
        // @TEMPORARY
#if 1
        void *base_address = TB(2);
#else
        LPVOID base_address = 0;
#endif
        Game_Memory game_memory = {};
        Linux_State linux_state = {};
        game_memory.permanent_memory_size = MB(256);
        game_memory.transient_memory_size = GB(1);
        game_memory.debug_storage_size    = MB(64);

        u64 total_capacity = (game_memory.permanent_memory_size +
                              game_memory.transient_memory_size + 
                              game_memory.debug_storage_size);
        linux_state.game_memory = mmap(0, total_capacity, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
        linux_state.game_memory_size = total_capacity;
        game_memory.permanent_memory = linux_state.game_memory;
        game_memory.transient_memory = ((u8 *)(game_memory.permanent_memory) + game_memory.permanent_memory_size);
        game_memory.debug_storage = ((u8 *)(game_memory.transient_memory) + game_memory.transient_memory_size);

#if 0 // @NOTE: Temporarily commented out.
        game_memory.high_priority_queue = &high_priority_queue;
        game_memory.low_priority_queue = &low_priority_queue;

        Platform_Api *platform = &game_memory.platform;
        platform->platform_add_entry = Win32AddEntry;
        platform->platform_complete_all_work = win32_complete_all_work;
        platform->debug_platform_read_file = win32_read_entire_file;
#endif

        Game_State *game_state = (Game_State *)game_memory.permanent_memory;
    } else {
        // @TODO: Error handling.
    }
}
