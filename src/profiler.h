/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Sung Woo Lee $
   $Notice: (C) Copyright 2025 by Sung Woo Lee. All Rights Reserved. $
   ======================================================================== */





struct Profile_Anchor {
    const char *label;
    u64 start_tsc;
    u64 elapsed;
    u32 hitcount;
};

struct Profiler {
    Profile_Anchor anchors[4096];
    u64 start_tsc;
    u64 end_tsc;
};

global Profiler g_profiler;

struct Profile_Block 
{
    u32 index;

    Profile_Block(const char *label, u32 index_)
    {
        Assert(index_ < array_count(g_profiler.anchors));
        index = index_;
        Profile_Anchor *anchor = g_profiler.anchors + index;
        anchor->label = label;
        anchor->start_tsc = os.read_cpu_timer();
        anchor->hitcount++;
    }

    ~Profile_Block()
    {
        Profile_Anchor *anchor = g_profiler.anchors + index;
        anchor->elapsed += os.read_cpu_timer() - anchor->start_tsc;
    }
};

internal f32
cpu_cycle_to_ms(u64 cycle)
{
    return 1000.0f * (f32)cycle / (f32)os.cpu_timer_frequency;
}

internal void
begin_profile()
{
    zero_struct(&g_profiler);
    g_profiler.start_tsc = os.read_cpu_timer();
}

internal void
end_profile()
{
    g_profiler.end_tsc = os.read_cpu_timer();
    u64 total_elapsed = g_profiler.end_tsc - g_profiler.start_tsc;
    f32 total_elapsed_ms = cpu_cycle_to_ms(total_elapsed);
}

#define TIME_BLOCK(name) Profile_Block CONCAT(block, __LINE__)(name, __COUNTER__ + 1)
#define TIME_FUNCTION() TIME_BLOCK(__func__)
