// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RTS_ARRAY_H
#define RTS_ARRAY_H

template <typename Type>
struct Array {
    Type *data;
    u64  count;
    u64  allocated;

    Allocator allocator;

    Type& operator [] (u64 idx);
    const Type& operator [] (u64 idx) const;

    Type *begin() { return data;         }
    Type *end()   { return data + count; }
};


// Adds item to the end. Unless you set the allocator yourself, it uses the context's allocator.
template <typename T>
internal void array_add(Array<T> *arr, T item);

// Reserves memory up to desired count. Calls 'Realloc' internally.
template <typename T>
internal void array_reserve(Array<T> *arr, u64 desired_count);

// Sets the count field to 0.
template <typename T>
internal void array_reset_keeping_memory(Array<T> *arr);

// Sets the count field to 0, and frees memory.
template <typename T>
internal void array_reset(Array<T> *arr);




#endif // RTS_ARRAY_H
