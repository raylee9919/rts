/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Seong Woo Lee $
   $Notice: (C) Copyright %s by Seong Woo Lee. All Rights Reserved. $
   ======================================================================== */

internal Random_Series
rand_seed(u32 seed) 
{
    Random_Series result = {};
    result.state = random_table[seed % array_count(random_table)];
    return result;
}

internal u32
rand_next(Random_Series *series)
{
#if 0
    u32 result = random_table[series->next_idx++];
    if (series->next_idx > array_count(random_table)) 
    {
        series->next_idx = 0;
    }
    return result;
#else
    u32 x = series->state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    series->state = x;
    return x;
#endif
}

internal f32
rand_unilateral(Random_Series *series)
{
#if 0
    f32 d = (f32)(max_in_random_table - min_in_random_table);
    f32 r = (f32)rand_next(series);
    f32 t = safe_ratio(r - min_in_random_table, d);
    f32 result = lerp(0.0f, 1.0f, t);
    return result;
#else
    f32 d = (f32)(0xffffffff);
    f32 r = (f32)rand_next(series);
    f32 t = r / d;
    f32 result = lerp(0.0f, 1.0f, t);
    return result;
#endif
}

internal f32
rand_bilateral(Random_Series *series)
{
    f32 result = rand_unilateral(series) * 2.0f - 1.0f;
    return result;
}

internal v3
rand_v3(Random_Series *series, f32 min, f32 max) 
{
    v3 result;
    for (int i = 0; i < 3; ++i) {
        result.e[i] = rand_unilateral(series) * (max-min) + min;
    }
    return result;
}

internal v3
rand_v3(Random_Series *series, v3 min, v3 max) 
{
    v3 result;
    for (int i = 0; i < 3; ++i) {
        result.e[i] = rand_unilateral(series) * (max.e[i]-min.e[i]) + min.e[i];
    }
    return result;
}

internal f32
rand_range(Random_Series *series, f32 lo, f32 hi)
{
    f32 result = lerp(lo, rand_unilateral(series), hi);
    return result;
}
