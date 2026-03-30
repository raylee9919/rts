// Copyright Seong Woo Lee. All Rights Reserved.

#pragma once


struct Animation_Player;

struct Pose_Channel {

    int index;
    Animation_Player* my_animation_player;

    Animation* animation;

    f32 current_t;
    f32 current_dt;
    f32 multiplier;

    bool active;
    bool loop;
    bool ended;




    void init(int channel_index, Animation_Player *player);
    void set_animation(Animation* anim, bool do_loop);
    void accumulate(f32 dt);
    void eval();
};

struct Animation_Player {

    Animation_Player* next;
    Animation_Player* prev;

    Skeleton* skeleton;

    m3x4 *skinning_matrices;

    Array <m4x4> blended_local_transforms;

    Pose_Channel channels[4];
    Array <Xform> local_transforms[4];

    f32 blend_weights[4];




    void init(Skeleton *skel, m3x4 *out_skinning_matrices);
    void accumulate(f32 dt);
    void eval();
};


Animation_Player* alloc_animation_player();
void release_animation_player(Animation_Player* player);
