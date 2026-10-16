// Copyright Seong Woo Lee. All Rights Reserved.

#include "./animation.h"
#include "basic/string_builder.h"

String 
asset_string_from_animation(Sampled_Animation *anim, 
                            Allocator          allocator)
{
  String s = {};

  String_Builder b = {};
  init(&b, allocator);

  append(&b, S("1 // version\n\n"));

  R_ASSERT(anim->skeleton != NULL_GUID);
  append(&b, tprint(S("@\"%S\"\n"), asset_name_from_guid(anim->skeleton)));
  append(&b, tprint(S("%d\n"), anim->num_joints));
  append(&b, S("\n"));

  append(&b, tprint(S("%d\n"), anim->frame_rate));
  append(&b, tprint(S("%.6f\n"), anim->duration));
  append(&b, S("\n"));

  append(&b, tprint(S("%u\n"), anim->samples.count));
  for (u32 i = 0; i < anim->samples.count; ++i)
  {
    Animation_Sample sample = anim->samples.data[i];
    for (u32 j = 0; j < sample.xforms.count; ++j)
    {
      Xform *xform = sample.xforms.data + j;
      vec3 t = xform->translation;
      Quaternion r = xform->rotation;
      vec3 s = xform->scale;
      append(&b, tprint(S("%.6f %.6f %.6f\n"), t.x, t.y, t.z));
      append(&b, tprint(S("%.6f %.6f %.6f %.6f\n"), r.w, r.x, r.y, r.z));
      append(&b, tprint(S("%.6f %.6f %.6f\n"), s.x, s.y, s.z));
      append(&b, S("\n"));
    }
  }

  s = flush(&b);
  return s;
}

// @Correctness
static m4x4 
m4x4_from_xform(Xform xform)
{
  m4x4 m;

  vec3 t       = xform.translation;
  Quaternion r = xform.rotation;
  vec3 s       = xform.scale;

  __m128 scale = _mm_setr_ps(s.x, s.y, s.z, 1.f);

  m._11 = 1.0f - 2.0f * (r.y * r.y + r.z * r.z);
  m._12 = 2.0f * (r.x * r.y - r.z * r.w);
  m._13 = 2.0f * (r.x * r.z + r.y * r.w);
  m._14 = t.x;
  m.rows[0].sse = _mm_mul_ps(m.rows[0].sse, scale);

  m._21 = 2.0f * (r.x * r.y + r.z * r.w);
  m._22 = 1.0f - 2.0f * (r.x * r.x + r.z * r.z);
  m._23 = 2.0f * (r.y * r.z - r.x * r.w);
  m._24 = t.y;
  m.rows[1].sse = _mm_mul_ps(m.rows[1].sse, scale);

  m._31 = 2.0f * (r.x * r.z - r.y * r.w);
  m._32 = 2.0f * (r.y * r.z + r.x * r.w);
  m._33 = 1.0f - 2.0f * (r.x * r.x + r.y * r.y);
  m._34 = t.z;
  m.rows[2].sse = _mm_mul_ps(m.rows[2].sse, scale);

  m.rows[3].sse = _mm_setr_ps(0, 0, 0, 1);

  return m;
}

// @Correctness
static Xform 
xform_from_m4x4(m4x4 m)
{
  Xform xform;

  vec3 translation = vec3(m._14, m._24, m._34);
  vec3 scale       = vec3(length(vec3(m._11, m._21, m._31)),
                          length(vec3(m._12, m._22, m._32)),
                          length(vec3(m._13, m._23, m._33)));

  m4x4 r = m;
  r._11 /= scale.x;
  r._21 /= scale.x;
  r._31 /= scale.x;

  r._12 /= scale.y;
  r._22 /= scale.y;
  r._32 /= scale.y;

  r._13 /= scale.z;
  r._23 /= scale.z;
  r._33 /= scale.z;

  // Rotation matrix to quaternion (Shepperd's method)
  // Row-major, so r._rc means row r, col c
  float trace = r._11 + r._22 + r._33;
  Quaternion rotation;

  if (trace > 0.0f)
  {
    float s = 0.5f / m_sqrt(trace + 1.0f);
    rotation.w = 0.25f / s;
    rotation.x = (r._32 - r._23) * s;
    rotation.y = (r._13 - r._31) * s;
    rotation.z = (r._21 - r._12) * s;
  }
  else if (r._11 > r._22 && r._11 > r._33)
  {
    float s = 2.0f * sqrtf(1.0f + r._11 - r._22 - r._33);
    rotation.w = (r._32 - r._23) / s;
    rotation.x = 0.25f * s;
    rotation.y = (r._12 + r._21) / s;
    rotation.z = (r._13 + r._31) / s;
  }
  else if (r._22 > r._33)
  {
    float s = 2.0f * sqrtf(1.0f + r._22 - r._11 - r._33);
    rotation.w = (r._13 - r._31) / s;
    rotation.x = (r._12 + r._21) / s;
    rotation.y = 0.25f * s;
    rotation.z = (r._23 + r._32) / s;
  }
  else
  {
    float s = 2.0f * sqrtf(1.0f + r._33 - r._11 - r._22);
    rotation.w = (r._21 - r._12) / s;
    rotation.x = (r._13 + r._31) / s;
    rotation.y = (r._23 + r._32) / s;
    rotation.z = 0.25f * s;
  }

  xform.translation = translation;
  xform.rotation    = rotation;
  xform.scale       = scale;

  return xform;
}
