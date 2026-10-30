// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RTS_ANIMATION_H
#define RTS_ANIMATION_H

#include "basic/core.h"
#include "mesh.h"

struct Xform
{
  vec3        translation;
  Quaternion  rotation;
  vec3        scale;

  force_inline Xform() = default;
  force_inline Xform(vec3 t, Quaternion r, vec3 s) : translation(t), rotation(r), scale(s) {}
};

struct Animation_Sample
{
  Array<Xform> xforms; // [num_nodes] Xform
};

struct Sampled_Animation
{
  Guid skeleton;
  s32  num_joints;

  s32 frame_rate; // frames per second
  f32 duration;

  Array<Animation_Sample> samples;
};

ASSET_LOAD_PROC(animation_load_proc);

String 
asset_string_from_animation(Sampled_Animation *anim, 
                            Allocator          allocator);

Sampled_Animation *
animation_from_guid(Guid id);

m4x4 
m4x4_from_xform(Xform *xform);

#endif // RTS_ANIMATION_H
