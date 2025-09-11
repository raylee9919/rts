/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Seong Woo Lee $
   $Notice: (C) Copyright %s by Seong Woo Lee. All Rights Reserved. $
   ======================================================================== */

#if __MSVC
  #include <intrin.h>
#elif __LINUX
  #include <x86intrin.h>
#endif


internal u32
get_thread_id()
{
#if __MSVC
    u8 *thread_local_storage = (u8 *)__readgsqword(0x30);
    u32 thread_id = *(u32 *)(thread_local_storage + 0x48);
#elif __LINUX
    u32 thread_id = 0;
#endif
    return thread_id;
}

internal f32 pow(f32 x, f32 y) {
    __m128 val = _mm_pow_ps(_mm_set1_ps(x), _mm_set1_ps(y));
    f32 result = *(f32 *)&val;
    return result;
}

internal f32 cos(f32 x) {
    __m128 val = _mm_cos_ps(_mm_set1_ps(x));
    f32 result = *(f32 *)&val;
    return result;
}

internal f32 acos(f32 x) {
    __m128 val = _mm_acos_ps(_mm_set1_ps(x));
    f32 result = *(f32 *)&val;
    return result;
}

internal f32 sin(f32 x) {
    __m128 val = _mm_sin_ps(_mm_set1_ps(x));
    f32 result = *(f32 *)&val;
    return result;
}

internal f32 tan(f32 x) {
    __m128 val = _mm_tan_ps(_mm_set1_ps(x));
    f32 result = *(f32 *)&val;
    return result;
}

internal f32 sqrt(f32 x) {
    __m128 val = _mm_sqrt_ps(_mm_set1_ps(x));
    f32 result = *(f32 *)&val;
    return result;
}

internal s32 round_f32_to_s32(f32 x) {
    s32 result = _mm_cvtss_si32(_mm_set_ss(x));
    return result;
}

internal u32 round_f32_to_u32(f32 x) {
    s32 result = (u32)_mm_cvtss_si32(_mm_set_ss(x));
    return result;
}
