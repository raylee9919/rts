// Copyright Seong Woo Lee. All Rights Reserved.


void Pose_Channel::init(int channel_index, Animation_Player* player) {
    index = channel_index;
    my_animation_player = player;
}

void Pose_Channel::set_animation(Animation* anim, bool do_loop) {
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

void Pose_Channel::accumulate(f32 dt) {
    if (ended) {
        return;
    }

    f32 t = dt * multiplier;
    current_t += t;
    current_dt = t;
}

static Animation_Joint* animation_node_from_joint_id(Animation *anim, s32 id) {
    u64 slot = hash_joint_id(id) % anim->table_size;
    auto *entry = anim->joint_table + slot;
    for (auto *link = entry->first; link; link = link->next) {
        if (link->joint->id == id) {
            return link->joint;
        }
    }

    return nullptr;
}

void Pose_Channel::eval() {
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

            m4x4 local_transform = joint->local_transform;

            if (node) {
                Xform *sample = &node->keyframes[anim->num_keyframes - 1];
                local_transform = m4x4_from_trs(sample->translation, sample->rotation, sample->scale);
            }

            // Write
            my_animation_player->local_transforms[index][ji] = local_transform;
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

            if (node) {
                Xform *sample1 = &node->keyframes[idx1];
                Xform *sample2 = &node->keyframes[idx2];

                Quaternion rot1 = sample1->rotation;
                Quaternion rot2 = sample2->rotation;
                if (dot(rot1, rot2) < 0.f) {
                    rot2 = -rot2;
                }
                Quaternion rotation = nlerp(rot1, t, rot2);
                v3 translation      = lerp(sample1->translation, t, sample2->translation);
                v3 scale            = lerp(sample1->scale, t, sample2->scale);

                local_transform = m4x4_from_trs(translation, rotation, scale);
            }

            // Write
            my_animation_player->local_transforms[index][ji] = local_transform;
        }
    }

}

void Animation_Player::init(Skeleton *skel) {
    skeleton = skel;

    skinning_matrices.reserve_to(skeleton->num_joints);
    blended_local_transforms.reserve_to(skeleton->num_joints);

    int num_channels = array_count(channels);
    for (int i = 0; i < num_channels; ++i) {
        auto* channel = &channels[i];
        channel->init(i, this);
        
        local_transforms[i].reserve_to(skeleton->num_joints);

        blend_weights[i] = 0.f;
    }
}

void Animation_Player::eval() {
    int num_channels = array_count(channels);
    int num_joints = skeleton->num_joints;

    // First, zero out the intermediate matrices which the weighted transforms will be piled upon.
    //
    memset(blended_local_transforms.data, 0, sizeof(blended_local_transforms.data[0]) * num_joints);

    for (int i = 0; i < num_channels; ++i) {
        channels[i].eval();
    }


    // Gather blending weights sum.
    //
    f32 weights_sum = 0.f;
    for (int ch = 0; ch < num_channels; ++ch) {
        if (channels[ch].active) {
            weights_sum += blend_weights[ch];
        }
    }
    if (weights_sum < 0.000001f) {
        return;
    }
    f32 denom = 1.f / weights_sum; // normalize


    // fmadd with blend weight
    //
    for (int ch = 0; ch < num_channels; ++ch) {
        if (channels[ch].active) {
            for (int j = 0; j < num_joints; ++j) {
                blended_local_transforms[j] += blend_weights[ch] * local_transforms[ch][j];
            }
        }
    }

    for (int j = 0; j < num_joints; ++j) {
        blended_local_transforms[j] *= denom;
    }


    // Build composite skinning matrices.
    for (int ji = 0; ji < num_joints; ++ji) {
        Joint *joint = &skeleton->joints[ji];
        int parent = joint->parent;

        if (parent >= 0) {
            blended_local_transforms[ji] = blended_local_transforms[parent] * blended_local_transforms[ji];
        }

        skinning_matrices[ji] = blended_local_transforms[ji] * joint->inverse_bind_pose;
    }
}

void Animation_Player::accumulate(f32 dt) {
    for (int ch = 0; ch < array_count(channels); ++ch) {
        channels[ch].accumulate(dt);
    }
}
