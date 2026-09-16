// Copyright Seong Woo Lee. All Rights Reserved.

#include "audio.h"
#include "basic/string.h"
#include "os/os.h"

void audio_entry(void *param)
{
    thread_set_name(S("RenderThread"));

    // @Todo
}
