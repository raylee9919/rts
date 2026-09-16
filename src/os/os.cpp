// Copyright Seong Woo Lee. All Rights Reserved.

#include "os/os.h"
#include "basic/string.h"
#include "third_party/xxhash3/xxhash.h"


void input_per_frame_event_and_flag_update() {
    array_reset_keeping_memory(&os->events);

    u32 mask     = ~KEY_STATE_START;
    u32 end_mask = ~(KEY_STATE_END | KEY_STATE_DOWN | KEY_STATE_START);

    for (auto& it : os->input_button_states) {
        if (it & KEY_STATE_END) {
            it &= end_mask;
        } else {
            it &= mask;
        }
    }

    os->mouse_delta_x = 0;
    os->mouse_delta_y = 0;
    os->mouse_delta_z = 0;
}

Guid guid_from_bytes(void *bytes, u64 size) {
    XXH128_hash_t hash = XXH3_128bits_withSeed(bytes, size, 0);
    Guid result;
    result._64[0] = hash.low64;
    result._64[1] = hash.high64;
    return result;
}

Guid guid_from_string(String str) {
    return guid_from_bytes(str.str, str.len * sizeof(str.str[0]));
}
