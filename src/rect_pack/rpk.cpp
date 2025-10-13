/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Seong Woo Lee $
   $Notice: (C) Copyright %s by Seong Woo Lee. All Rights Reserved. $
   ======================================================================== */


internal Rpk_Segment *
rpk_alloc_segment(Rpk_Context *ctx)
{
    Assert(ctx->initted);

    Rpk_Segment *result = ctx->first_free_segment;
    if (! result) 
    {
        result = push_struct(ctx->arena, Rpk_Segment);
    } 
    else 
    {
        zero_memory(result, sizeof(Rpk_Segment));
        sll_pop_front(ctx->first_free_segment, ctx->last_free_segment);
    }
    return result;
}

internal void
rpk_release_segment(Rpk_Context *ctx, Rpk_Segment *segment)
{
    Assert(ctx->initted);

    sll_push_back(ctx->first_free_segment, ctx->last_free_segment, segment);
}

internal void
rpk_init_segment(Rpk_Segment *segment, u32 x, u32 y, u32 w)
{
    Assert(segment);

    segment->x = x;
    segment->y = y;
    segment->w = w;
}

internal void
rpk_init(Rpk_Context *ctx, Arena *arena, u32 w, u32 h)
{
    ctx->initted = true;
    ctx->arena = arena;
    ctx->w = w;
    ctx->h = h;

    Rpk_Segment *sg = rpk_alloc_segment(ctx); 
    sg->x = 0;
    sg->y = 0;
    sg->w = w;

    dll_push_back(ctx->segment_first, ctx->segment_last, sg);
}

internal int
rpk_seg_cmp(void *s1, void *s2)
{
    Rpk_Segment *a = (Rpk_Segment *)s1;
    Rpk_Segment *b = (Rpk_Segment *)s2;

    if ( (a->y > b->y) || ((a->y == b->y) && (a->x > b->x)) ) {
        return true;
    }
    return false;
}

internal Rpk_Result
rpk_do(Rpk_Context *ctx, u32 w, u32 h)
{
    Assert(ctx->initted);
    Rpk_Result result = {};

    dll_sort(ctx->segment_first, ctx->segment_last, Rpk_Segment, rpk_seg_cmp);

    for (Rpk_Segment *seg = ctx->segment_first, *next; seg != NULL; seg = next)
    {
        next = seg->next;

        u32 lx = seg->x;
        u32 rx = seg->x + w;
        u32 y = seg->y;
        u32 end_y = y + h;
        // @Note: Since the segments are sorted in ascending y, there's no need to
        //        iterate further once we exceeded the maximum y. It won't fit.
        if (end_y > ctx->h)
        { break; }

        u32 lim_x = ctx->w;
        for (Rpk_Segment *end = seg->next; end != NULL; end = end->next)
        {
            if (end->x > lx)
            {
                lim_x = min(lim_x, end->x);
            }
        }

        if (rx <= lim_x)
        {
            result.fit = true;
            result.x = lx;
            result.y = y;

            Rpk_Segment *prev_store = 0;
            for (Rpk_Segment *below = seg; below != NULL; below = prev_store)
            {
                prev_store = below->prev;
                u32 blx = below->x;
                u32 brx = below->x + below->w;

                if (brx > lx && blx < rx) // overlap
                {
                    if (brx <= rx) // includes
                    {
                        dll_remove(ctx->segment_first, ctx->segment_last, below);
                        rpk_release_segment(ctx, below);
                    }
                    else
                    {
                        below->x = rx;
                        below->w = (brx - rx);
                    }
                }
            }

            Rpk_Segment *seg_new = rpk_alloc_segment(ctx);
            rpk_init_segment(seg_new, lx, end_y, w);
            dll_push_back(ctx->segment_first, ctx->segment_last, seg_new);

            break;
        }
    }

    return result;
}
