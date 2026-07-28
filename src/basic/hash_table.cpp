// Copyright Seong Woo Lee. All Rights Reserved.

template <typename K, typename V, u32 (*H)(K), u32 L>
static V *table_add(Table<K, V, H, L> *table, K key, V value) {
    if ((table->slots_filled + 1) * 100 >= table->allocated * TABLE_LOAD_FACTOR_PERCENT) {

        s64 new_allocated = table->allocated;

        if ((table->count * 2 + 1) * 100 >= table->allocated * TABLE_LOAD_FACTOR_PERCENT) {
            new_allocated = table->allocated * 2;
        }

        new_allocated = max(new_allocated, TABLE_SIZE_MIN);

        // Must be a power of two.
        Assert((new_allocated & (new_allocated - 1)) == 0);

        auto old_entries = table->entries;

        if (!table->allocator.proc) {
            table->allocator = tctx.allocator;
        }

        table->entries = Array<Table<K, V, H, L>::Entry>{};
        array_reserve(&table->entries, new_allocated);

        // 'count' and 'slots_filled' will be incremented by 'table_add'.
        table->count        = 0;
        table->slots_filled = 0;
        table->allocated    = new_allocated;

        for (auto i = 0; i < old_entries.allocated; ++i) {
            auto entry = old_entries[i];
            if (entry.hash >= HASH_FIRST_VALID) {
                table_add(table, entry.key, entry.value);
            }
        }

        dealloc(old_entries.data, table->allocator);
    }

    Assert(table->slots_filled < table->allocated);

    //
    // Walk through the table and add the key-value pair.
    //
    u32 mask = (u32)(table->allocated - 1); // @Todo: sus

    u32 hash = table->hash(key);
    if (hash < HASH_FIRST_VALID) hash += HASH_FIRST_VALID;

    s64 index = hash & mask;

    u32 probe_increment = 1;

    while (u32 h = table->entries[index].hash) {
        auto *entry = &table->entries[index];

        // Refill
        if (entry->hash == HASH_REMOVED) {
            table->slots_filled -= 1; // 1 will be re-added below for total increment 0.
            break;
        }

        index = (index + probe_increment) & mask;
        probe_increment += 1;
    }

    table->count        += 1;
    table->slots_filled += 1;

    auto *entry = &table->entries[index];
    entry->hash  = hash;
    entry->key   = key;
    entry->value = value;

    return &entry->value;
}

template <typename K, typename V, u32 (*H)(K), u32 L>
static bool table_remove(Table<K, V, H, L> *table, K key) {
    if (!table->allocated) return false;

    u32 mask = (u32)(table->allocated - 1); // @Todo: sus

    u32 hash = table->hash(key);
    if (hash < HASH_FIRST_VALID) hash += HASH_FIRST_VALID;

    s64 index = hash & mask;

    u32 probe_increment = 1;

    while (u32 h = table->entries[index].hash) {
        auto *entry = &table->entries[index];

        if ((entry->hash == hash) && (entry->key == key)) {
            entry->hash = HASH_REMOVED;
            table->count -= 1;
            return true;
        }

        index = (index + probe_increment) & mask;
        probe_increment += 1;
    }

    return false;
}

template <typename K, typename V, u32 (*H)(K), u32 L>
static void table_reset(Table<K, V, H, L> *table) {
    auto old_allocator = table->allocator;
    dealloc(table->entries.data, table->allocator);
    memset(table, 0, sizeof(*table));
    table->allocator = old_allocator;
}

template <typename K, typename V, u32 (*H)(K), u32 L>
static void table_reset_keeping_memory(Table<K, V, H, L> *table) {
    table->count        = 0;
    table->slots_filled = 0;

    for (s64 i = 0; i < table->allocated; ++i) {
        auto *entry = &table->entries[i];
        entry->hash = 0;
    }
}

template <typename K, typename V, u32 (*H)(K), u32 L>
static V *table_find_pointer(Table<K, V, H, L> *table, K key) {
    if (!table->allocated) return nullptr;

    {
        u32 mask = (u32)(table->allocated - 1); // @Todo: sus

        u32 hash = table->hash(key);
        if (hash < HASH_FIRST_VALID) hash += HASH_FIRST_VALID;

        s64 index = hash & mask;

        u32 probe_increment = 1;

        while (u32 h = table->entries[index].hash) {
            auto *entry = &table->entries[index];

            if ((entry->hash == hash) && (entry->key == key)) {
                return &entry->value;
            }

            index = (index + probe_increment) & mask;
            probe_increment += 1;
        }
    }

    return nullptr;
}

template <typename K, typename V, u32 (*H)(K), u32 L>
static Table_Find_Result<V> table_find(Table<K, V, H, L> *table, K key) {
    Table_Find_Result<V> result = {};

    V *ptr = table_find_pointer(table, key);
    if (ptr) {
        result.found = true;
        result.value = *ptr;
    }

    return result;
}
