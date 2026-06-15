// Copyright Seong Woo Lee. All Rights Reserved.


void Pose_Channel::init(int channel_index, Animation_Player* player) 
{
    index = channel_index;
    my_animation_player = player;
}

void Pose_Channel::set_animation(Animation* anim, bool do_loop) 
{
    if (animation == anim) {
        return;
    }

    animation = anim;

    current_t  = 0.f;
    current_dt = 0.f;
    multiplier = 1.f;

    active = true;
    loop   = do_loop;
    ended  = false;
}

void Pose_Channel::accumulate(f32 dt) 
{
    if (ended) {
        return;
    }

    f32 t = dt * multiplier;
    current_t += t;
    current_dt = t;
}

internal Animation_Joint* animation_node_from_joint_id(Animation* anim, s32 id) 
{
    u64 slot = hash_joint_id(id) % anim->table_size;
    auto *entry = anim->joint_table + slot;
    for (auto *link = entry->first; link; link = link->next) {
        if (link->joint->id == id) {
            return link->joint;
        }
    }

    return NULL;
}

void Pose_Channel::eval() 
{
    if (!animation || !active || ended) {
        return;
    }

    Skeleton *skel  = my_animation_player->skeleton;
    u32 num_joints  = skel->num_joints;
    Animation *anim = animation;
    u32 num_nodes   = anim->num_joints;

    current_t = fmod_cycling(current_t, anim->duration);

    const f32 fps = (f32)anim->num_keyframes / anim->duration;

    u32 idx1 = (u32)(current_t * fps) % anim->num_keyframes;
    if ((idx1 == anim->num_keyframes - 1) && !loop) {

        for (u32 ji = 0; ji < num_joints; ++ji) {
            Joint *joint = &skel->joints[ji];
            s32 parent = joint->parent;

            Animation_Joint *node = animation_node_from_joint_id(anim, (s32)ji);

            Xform xform = joint->local_xform;
            if (node) {
                xform = node->keyframes[anim->num_keyframes - 1];
            }

            // Write
            my_animation_player->local_transforms[index][ji] = xform;
        }

        ended = true;
    } else {
        u32 idx2 = (idx1 + 1) % anim->num_keyframes;

        f32 t = current_t * fps - idx1;

        for (u32 ji = 0; ji < num_joints; ++ji) {
            Joint *joint = &skel->joints[ji];
            s32 parent = joint->parent;

            Animation_Joint *node = animation_node_from_joint_id(anim, (s32)ji);

            m4x4 local_transform = joint->local_transform;

            Xform xform = joint->local_xform;

            if (node) {
                Xform *sample1 = &node->keyframes[idx1];
                Xform *sample2 = &node->keyframes[idx2];

                Quaternion rot1 = sample1->rotation;
                Quaternion rot2 = sample2->rotation;
                if (dot(rot1, rot2) < 0.f) {
                    rot2 = -rot2;
                }
                Quaternion rotation = slerp(rot1, t, rot2);
                v3 translation      = lerp(sample1->translation, t, sample2->translation);
                v3 scale            = lerp(sample1->scale, t, sample2->scale);

                xform.translation = translation;
                xform.rotation    = rotation;
                xform.scale       = scale;
            }

            // Write
            my_animation_player->local_transforms[index][ji] = xform;
        }
    }
}

void Animation_Player::init(Skeleton *skel, m3x4 *out_skinning_matrices) 
{
    skeleton = skel;

    assert(out_skinning_matrices);
    skinning_matrices = out_skinning_matrices;

    blended_local_transforms.reserve_to(skeleton->num_joints);

    int num_channels = array_count(channels);
    for (int i = 0; i < num_channels; ++i) {
        auto* channel = &channels[i];
        channel->init(i, this);
        
        local_transforms[i].reserve_to(skeleton->num_joints);

        blend_weights[i] = 0.f;
    }
}

void Animation_Player::eval() 
{
    ProfileScope;

    int num_channels = array_count(channels);
    int num_joints = skeleton->num_joints;

    // First, clear the intermediate matrices that will accumulate the weighted transforms.
    memset(blended_local_transforms.data, 0, sizeof(blended_local_transforms.data[0]) * num_joints);

    for (int i = 0; i < num_channels; ++i) {
        channels[i].eval();
    }


    // Gather blending weights sum.
    f32 weights_sum = 0.f;
    for (int ch = 0; ch < num_channels; ++ch) {
        if (channels[ch].active) {
            weights_sum += blend_weights[ch];
        }
    }

    // Calculate reciprocal total weight sum.
    f32 rcp_weights = 1.f;
    if (weights_sum > 1e-8f) {
        rcp_weights = 1.f / weights_sum;
    }

    // fmadd with blend weight

    for (int j = 0; j < num_joints; ++j) {
        v3 translation      = v3(0.f);
        Quaternion rotation = Quaternion(0.f, 0.f, 0.f, 0.f);
        v3 scale            = v3(0.f);

        for (int ch = 0; ch < num_channels; ++ch) {
            if (channels[ch].active) {
                Xform xform = local_transforms[ch][j];
                f32 weight = blend_weights[ch] * rcp_weights;
                translation += xform.translation * weight;
                if (dot(rotation, xform.rotation) < 0.f) {
                    xform.rotation = -xform.rotation;
                }
                rotation    = rotation + xform.rotation * weight;
                scale       += xform.scale * weight;
            }
        }

        rotation = normalize(rotation);

        m4x4 bone_matrix = to_m4x4(translation, rotation, scale);
        blended_local_transforms[j] = bone_matrix;
    }


    // Build composite skinning matrices.
    for (int ji = 0; ji < num_joints; ++ji) {
        Joint *joint = &skeleton->joints[ji];
        int parent = joint->parent;

        if (parent >= 0) {
            blended_local_transforms[ji] = blended_local_transforms[parent] * blended_local_transforms[ji];
        }

        m4x4 m = skeleton->root_transform * blended_local_transforms[ji] * joint->inverse_bind_pose;
        memory_copy(&skinning_matrices[ji], &m, sizeof(m3x4));
    }
}

void Animation_Player::accumulate(f32 dt) {
    for (int ch = 0; ch < array_count(channels); ++ch) {
        channels[ch].accumulate(dt);
    }
}



// Allocation / Release
//
Animation_Player* alloc_animation_player() 
{
    auto gs = game_state;

    Arena* arena = gs->animation_arena;
    Animation_Player* player = gs->first_free_animation_player;

    if (player) {
        sll_pop_front(gs->first_free_animation_player, gs->last_free_animation_player);
        zero_struct(player);
    } else {
        player = push_struct(arena, Animation_Player);
    }

    dll_push_back(gs->first_animation_player, gs->last_animation_player, player);

    return player;
}

void release_animation_player(Animation_Player* player) 
{
    auto gs = game_state;

    if (player) {
        dll_remove(gs->first_animation_player, gs->last_animation_player, player);
        sll_push_back(gs->first_free_animation_player, gs->last_free_animation_player, player);
    }
}
