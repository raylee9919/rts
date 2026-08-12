// Copyright Seong Woo Lee. All Rights Reserved.

//
// @Note: The implementation is largely based on Jai. 
// I couldn't resist borrowing from one of the best programmers on the planet.
//

#ifndef RTS_HASH_TABLE_H
#define RTS_HASH_TABLE_H


#define TABLE_SIZE_MIN              32
#define TABLE_LOAD_FACTOR_PERCENT   70


#define HASH_NEVER_OCCUPIED 0
#define HASH_REMOVED        1
#define HASH_FIRST_VALID    2


template <typename K, typename V, u32 (*given_hash_function)(K) = NULL, u32 LOAD_FACTOR_PERCENT = TABLE_LOAD_FACTOR_PERCENT>
struct Table {
    s64 count;        // The number of valid items in the table..
    s64 allocated;    // The numbers of slots for which we have allocated memory.
    s64 slots_filled; // The numbers of slots that can't be used for new items (either currently valid items or items that were removed).

    Allocator allocator;

    struct Entry {
        u32  hash;
        K    key;
        V    value;
    };

    Array<Entry> entries;

    u32 hash(K key) {
        if constexpr (given_hash_function) { // Because of this, I had to use C++17
            return given_hash_function(key);
        } else {
            return default_hash(key);
        }
    }
};

template <typename V>
struct Table_Find_Result {
    b32 found;
    V   value;
};


//
// Adds the given key value pair to the table, returns a pointer to the inserted value. 
// If you add a key twice, the table will not currently notice that this has happened,
// so you'll just get the first one. 
//
template <typename K, typename V, u32 (*H)(K), u32 L>
static V *table_add(Table<K, V, H, L> *table, K key, V value);

template <typename K, typename V, u32 (*H)(K), u32 L>
static bool table_remove(Table<K, V, H, L> *table, K key);

template <typename K, typename V, u32 (*H)(K), u32 L>
static void table_reset(Table<K, V, H, L> *table);

template <typename K, typename V, u32 (*H)(K), u32 L>
static void table_reset_keeping_memory(Table<K, V, H, L> *table);

template <typename K, typename V, u32 (*H)(K), u32 L>
static V *table_find_pointer(Table<K, V, H, L> *table, K key);

template <typename K, typename V, u32 (*H)(K), u32 L>
static Table_Find_Result<V> table_find(Table<K, V, H, L> *table, K key);


#endif // RTS_HASH_TABLE_H
