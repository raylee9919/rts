// Copyright Seong Woo Lee. All Rights Reserved.

#include "animation/animation.h"
#include "asset/asset.h"
#include "asset/parser.h"
#include "basic/context.h"
#include "basic/hash_table.h"
#include "basic/log.h"
#include "game.h"
#include "profiler/profiler.h"

namespace Asset
{
    // Values are pointers, animation players hold on to them across table growth.
    static Table<Guid, Skeleton*,  hash_guid> skeleton_table;
    static Table<Guid, Animation*, hash_guid> animation_table;

    static b32 read_file(Parser *p, String filepath)
    {
        String contents = read_entire_file(filepath, tctx.temp);
        if ( !contents.str ) {
            log_error(S("Couldn't read file: '%S'"), filepath);
            return false;
        }
        init(p, contents.str, contents.len);
        return true;
    }

    void skeleton_load_proc(String filepath, String short_name, void *user_data)
    {
        Allocator heap = asset_system->heap;
        skeleton_table.allocator = heap;

        Temporary_Arena scratch = scratch_begin();
        defer( scratch_end(scratch) );

        Parser p = {};
        if ( !read_file(&p, filepath) )  return;

        auto *skel = (Skeleton *)alloc(sizeof(Skeleton), heap);
        Construct(skel);

        skel->num_joints     = parse_u32(&p);
        skel->root_transform = parse_m4x4(&p);
        skel->joints         = (Joint *)alloc(sizeof(Joint) * skel->num_joints, heap);

        for (u32 ji = 0; ji < skel->num_joints; ++ji) {
            Joint *joint = &skel->joints[ji];
            Construct(joint);

            u8 name_len              = (u8)parse_u32(&p);
            joint->name              = copy_string(parse_string_by_length(&p, name_len, scratch.arena), heap);
            joint->parent            = parse_s32(&p);
            joint->local_transform   = parse_m4x4(&p);
            joint->inverse_bind_pose = parse_m4x4(&p);
            joint->local_xform       = to_xform(joint->local_transform);

            R_ASSERT( joint->parent < (s32)ji );
        }

        R_ASSERT( is_eof(&p) );

        log_info(S("Loaded skeleton '%S', %u joints."), short_name, skel->num_joints);

        table_add(&skeleton_table, guid_from_string(short_name), skel);
    }

    void animation_load_proc(String filepath, String short_name, void *user_data)
    {
        Allocator heap = asset_system->heap;
        animation_table.allocator = heap;

        Temporary_Arena scratch = scratch_begin();
        defer( scratch_end(scratch) );

        Parser p = {};
        if ( !read_file(&p, filepath) )  return;

        auto *anim = (Animation *)alloc(sizeof(Animation), heap);
        Construct(anim);

        u8 name_len         = (u8)parse_u32(&p);
        anim->name          = copy_string(parse_string_by_length(&p, name_len, scratch.arena), heap);
        anim->duration      = parse_f32(&p);
        anim->num_keyframes = parse_u32(&p);
        u32 num_joints      = parse_u32(&p);

        R_ASSERT( anim->duration > 1e-8f ); // It'll cause divide by zero.
        R_ASSERT( anim->num_keyframes > 0 );

        // Joint ids are parsed before we know the largest one, so gather first.
        s32    *ids       = push_array(scratch.arena, s32, num_joints);
        Xform  *keyframes = (Xform *)alloc(sizeof(Xform) * num_joints * anim->num_keyframes, heap);

        for (u32 ji = 0; ji < num_joints; ++ji) {
            ids[ji] = parse_s32(&p);
            R_ASSERT( ids[ji] >= 0 );
            anim->num_tracks = max(anim->num_tracks, (u32)ids[ji] + 1);

            for (u32 ki = 0; ki < anim->num_keyframes; ++ki) {
                Xform *k = &keyframes[ji * anim->num_keyframes + ki];
                k->translation = parse_v3(&p);
                k->rotation    = parse_quaternion(&p);
                k->scale       = parse_v3(&p);
            }
        }

        anim->tracks = (Xform **)alloc(sizeof(Xform *) * anim->num_tracks, heap);
        memset(anim->tracks, 0, sizeof(Xform *) * anim->num_tracks);
        for (u32 ji = 0; ji < num_joints; ++ji) {
            anim->tracks[ids[ji]] = &keyframes[ji * anim->num_keyframes];
        }

        R_ASSERT( is_eof(&p) );

        log_info(S("Loaded animation '%S', %u keyframes, %u joints."), short_name, anim->num_keyframes, num_joints);

        table_add(&animation_table, guid_from_string(short_name), anim);
    }

    Skeleton *skeleton_from_guid(Guid id)
    {
        Skeleton **result = table_find_pointer(&skeleton_table, id);
        return result ? *result : nullptr;
    }

    Animation *animation_from_guid(Guid id)
    {
        Animation **result = table_find_pointer(&animation_table, id);
        return result ? *result : nullptr;
    }
}

// ------------------------------------------------------------------------- //

u64 animation_player_alloc(Game_State *g, Asset::Skeleton *skeleton)
{
    R_ASSERT(skeleton);

    auto *player = (Animation_Player *)game_alloc(g, sizeof(Animation_Player), align_of(Animation_Player));
    memset(player, 0, sizeof(Animation_Player));
    player->skeleton = skeleton;

    auto *matrices = (m3x4 *)game_alloc(g, sizeof(m3x4) * skeleton->num_joints, align_of(m3x4));
    player->skinning_matrices = (u8 *)matrices - g->storage.base;

    // Start in the bind pose, in case the renderer gets it before the first update.
    for (u32 ji = 0; ji < skeleton->num_joints; ++ji) {
        memcpy(&matrices[ji], &skeleton->root_transform, sizeof(m3x4));
    }

    return (u8 *)player - g->storage.base;
}

Animation_Player *animation_player_from_offset(Game_State *g, u64 offset)
{
    if ( !offset )  return nullptr;
    return (Animation_Player *)(g->storage.base + offset);
}

m3x4 *animation_player_skinning_matrices(Game_State *g, Animation_Player *player)
{
    return (m3x4 *)(g->storage.base + player->skinning_matrices);
}

void animation_player_set(Animation_Player *player, u32 channel, Asset::Animation *anim, b32 loop, f32 weight)
{
    R_ASSERT(channel < ANIMATION_MAX_CHANNELS);

    player->blend_weights[channel] = weight;

    Pose_Channel *ch = &player->channels[channel];
    if (ch->animation == anim)  return;

    ch->animation  = anim;
    ch->current_t  = 0.f;
    ch->multiplier = 1.f;
    ch->active     = anim != nullptr;
    ch->loop       = loop;
    ch->ended      = false;
}

// Samples the channel's animation into 'out', one Xform per skeleton joint.
static void pose_channel_eval(Pose_Channel *ch, Asset::Skeleton *skel, Xform *out)
{
    Asset::Animation *anim = ch->animation;

    ch->current_t = fmod_cycling(ch->current_t, anim->duration);

    const f32 fps = (f32)anim->num_keyframes / anim->duration;

    u32 idx1 = (u32)(ch->current_t * fps) % anim->num_keyframes;
    u32 idx2 = (idx1 + 1) % anim->num_keyframes;
    f32 t    = ch->current_t * fps - idx1;

    b32 last = (idx1 == anim->num_keyframes - 1) && !ch->loop;
    if (last) {
        ch->ended = true;
    }

    for (u32 ji = 0; ji < skel->num_joints; ++ji) {
        Xform *track = (ji < anim->num_tracks) ? anim->tracks[ji] : nullptr;

        if (!track) {
            out[ji] = skel->joints[ji].local_xform;
        } else if (last) {
            out[ji] = track[anim->num_keyframes - 1];
        } else {
            Xform *sample1 = &track[idx1];
            Xform *sample2 = &track[idx2];

            Quaternion rot1 = sample1->rotation;
            Quaternion rot2 = sample2->rotation;
            if (dot(rot1, rot2) < 0.f) {
                rot2 = -rot2;
            }

            out[ji].translation = lerp(sample1->translation, sample2->translation, t);
            out[ji].rotation    = slerp(rot1, rot2, t);
            out[ji].scale       = lerp(sample1->scale, sample2->scale, t);
        }
    }
}

void animation_player_update(Game_State *g, Animation_Player *player, f32 dt)
{
    ProfileScope;

    if (player->updated_at == g->time)  return;
    player->updated_at = g->time;

    Temporary_Arena scratch = scratch_begin();
    defer( scratch_end(scratch) );

    Asset::Skeleton *skel = player->skeleton;
    u32 num_joints        = skel->num_joints;

    // Advance and sample each channel.
    Xform *poses[ANIMATION_MAX_CHANNELS] = {};
    f32 weights_sum = 0.f;

    for (u32 c = 0; c < ANIMATION_MAX_CHANNELS; ++c) {
        Pose_Channel *ch = &player->channels[c];
        if (!ch->active || !ch->animation)  continue;

        if (!ch->ended) {
            ch->current_t += dt * ch->multiplier;
        }

        poses[c] = push_array(scratch.arena, Xform, num_joints);
        pose_channel_eval(ch, skel, poses[c]);

        weights_sum += player->blend_weights[c];
    }

    f32 rcp_weights = (weights_sum > 1e-8f) ? 1.f / weights_sum : 1.f;

    // Blend channels into local transforms, then compose down the hierarchy.
    // Parents always precede their children, so a single pass does it.
    m4x4 *globals = push_array(scratch.arena, m4x4, num_joints);
    m3x4 *out     = animation_player_skinning_matrices(g, player);

    for (u32 ji = 0; ji < num_joints; ++ji) {
        Asset::Joint *joint = &skel->joints[ji];

        vec3       translation = vec3(0.f);
        Quaternion rotation    = Quaternion(0.f, 0.f, 0.f, 0.f);
        vec3       scale       = vec3(0.f);
        b32        any         = false;

        for (u32 c = 0; c < ANIMATION_MAX_CHANNELS; ++c) {
            if (!poses[c])  continue;

            Xform xform = poses[c][ji];
            f32 weight  = player->blend_weights[c] * rcp_weights;

            if (dot(rotation, xform.rotation) < 0.f) {
                xform.rotation = -xform.rotation;
            }

            translation += xform.translation * weight;
            rotation     = rotation + xform.rotation * weight;
            scale       += xform.scale * weight;
            any          = true;
        }

        m4x4 local = any ? to_m4x4(translation, normalize(rotation), scale) : joint->local_transform;

        globals[ji] = (joint->parent >= 0) ? globals[joint->parent] * local : local;

        m4x4 m = skel->root_transform * globals[ji] * joint->inverse_bind_pose;
        memcpy(&out[ji], &m, sizeof(m3x4));
    }
}
