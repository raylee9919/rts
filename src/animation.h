// Copyright Seong Woo Lee. All Rights Reserved.

#pragma once


struct Animation_Player;

struct Pose_Channel {

    int index = -1;
    Animation_Player *my_animation_player = nullptr;

    Animation* animation = nullptr;

    // @Temporary
    f32 duration;

    f32 current_t  = 0.f;
    f32 current_dt = 0.f;
    f32 multiplier = 0.f;

    bool active = false;
    bool loop   = false;
    bool ended  = false;




    void init(int channel_index, Animation_Player *player);
    void set_animation(Animation* anim, bool do_loop);
    void accumulate(f32 dt);
    void eval();
};

struct Animation_Player {

    Skeleton *skeleton = nullptr;

    Array <m4x4> skinning_matrices = {};
    Array <m4x4> blended_local_transforms = {};

    Pose_Channel channels[4] = {};
    Array <m4x4> local_transforms[4] = {};

    f32 blend_weights[4] = {};




    void init(Skeleton *skel);
    void eval();
};
