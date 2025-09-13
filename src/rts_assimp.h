/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Seong Woo Lee $
   $Notice: (C) Copyright %s by Seong Woo Lee. All Rights Reserved. $
   ======================================================================== */

#define malloc_type(type) (type *)malloc_internal(sizeof(type))
#define malloc_array(type, count) (type *)malloc_internal(sizeof(type)*count)
internal void *
malloc_internal(u64 size) 
{
    void *result = malloc(size);
    zero_memory(result, size);
    return result;
}

#define fwrite_item(item, file) fwrite(&item, sizeof(item), 1, file)
#define fwrite_array(ptr, count, file) fwrite(ptr, sizeof(ptr[0]) * count, 1, file)



// -----------------------------------------
// @Note: Hash Table
struct Hash_Slot 
{
    char *name;
    s32 node_id; // this isn't the offset starting from the hash table slots!
    Hash_Slot *next;
};
struct Hash_Entry 
{
    Hash_Slot *first;
};
struct Hashmap 
{
    Hash_Entry *entries;
    u64 length;
    s32 next_id;
};



// ----------------------------------------------------------
// @Note: Quick Sort Compare Function
internal b32 asmp_cmp_ascending_node_id(void *a, void *b);

// ----------------------------------------------------------
// @Note: Conversion
internal v3 v3_from_ai(aiVector3D ai_v);
internal Quaternion quaternion_from_ai(aiQuaternion ai_q);
internal m4x4 m4x4_from_ai(aiMatrix4x4 ai_mat);
