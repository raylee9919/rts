/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Seong Woo Lee $
   $Notice: (C) Copyright %s by Seong Woo Lee. All Rights Reserved. $
   ======================================================================== */

#define KB(X) (   X *  1024ll)
#define MB(X) (KB(X) * 1024ll)
#define GB(X) (MB(X) * 1024ll)
#define TB(X) (GB(X) * 1024ll)
#define assert(EXP) if (!(EXP)) { *(volatile int *)0 = 0; }

#define malloc_type(type) (type *)malloc_internal(sizeof(type))
#define malloc_array(type, count) (type *)malloc_internal(sizeof(type)*count)
static void *
malloc_internal(umm size) {
    void *result = malloc(size);
    zero_size(result, size);
    return result;
}

#define fwrite_item(item, file) fwrite(&item, sizeof(item), 1, file)
#define fwrite_array(ptr, count, file) fwrite(ptr, sizeof(ptr[0]) * count, 1, file)
