// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RTS_ANIMATION_H
#define RTS_ANIMATION_H

#include "basic/core.h"
#include "basic/string.h"
#include "math/math.h"
#include "os/os.h"

struct Game_State;

#define ANIMATION_MAX_CHANNELS 4

namespace Asset
{
    struct Joint {
        String name;
        s32    parent; // Always precedes the child. -1 for the root.

        m4x4   local_transform;
        Xform  local_xform;

        m4x4   inverse_bind_pose;
    };

    struct Skeleton {
        m4x4   root_transform; // Not the root joint's transform. Applied on top of the whole hierarchy.

        u32    num_joints;
        Joint *joints;
    };

    struct Animation {
        String  name;

        f32     duration;
        u32     num_keyframes;

        // tracks[joint id] is 'num_keyframes' samples, or null if the joint isn't animated.
        u32     num_tracks;
        Xform **tracks;
    };

    // Asset_Type_Info::load_proc for the 'skeleton' and 'keyframed_animation' extensions.
    void skeleton_load_proc(String filepath, String short_name, void *user_data);
    void animation_load_proc(String filepath, String short_name, void *user_data);

    // Null if never requested.
    Skeleton  *skeleton_from_guid(Guid id);
    Animation *animation_from_guid(Guid id);
}

struct Pose_Channel {
    Asset::Animation *animation;

    f32 current_t;
    f32 multiplier;

    b8  active;
    b8  loop;
    b8  ended;
};

// Lives in the game storage, so it's copied to the render thread along with the
// rest of the state. The asset pointers point outside of the storage, but assets
// are immutable once loaded, so that's fine.
struct Animation_Player {
    Asset::Skeleton *skeleton;

    Pose_Channel     channels[ANIMATION_MAX_CHANNELS];
    f32              blend_weights[ANIMATION_MAX_CHANNELS];

    // Storage offset to 'skeleton->num_joints' m3x4s. Read by the renderer.
    u64              skinning_matrices;

    // Game time of the last update. Several entities may share a player, it's
    // advanced only once per tick.
    f64              updated_at;
};

// Returns the storage offset, which is what goes in Entity::animation_player.
u64               animation_player_alloc(Game_State *g, Asset::Skeleton *skeleton);
Animation_Player *animation_player_from_offset(Game_State *g, u64 offset);
m3x4             *animation_player_skinning_matrices(Game_State *g, Animation_Player *player);

void              animation_player_set(Animation_Player *player, u32 channel, Asset::Animation *anim, b32 loop, f32 weight);
void              animation_player_update(Game_State *g, Animation_Player *player, f32 dt);


#endif // RTS_ANIMATION_H
