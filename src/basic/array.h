// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RTS_ARRAY_H
#define RTS_ARRAY_H

#include "basic/core.h"
#include "basic/allocator.h"
#include "basic/context.h"

template <typename Type>
struct Array {
    Type *data      = nullptr;
    u64  count      = 0;
    u64  allocated  = 0;

    Allocator allocator = {};

    Type& operator [] (u64 idx);
    const Type& operator [] (u64 idx) const;

    Type *begin() { return data;         }
    Type *end()   { return data + count; }
};


// Adds item to the end. Unless you set the allocator yourself, it uses the context's allocator.
template <typename T>
void array_add(Array<T> *arr, T item);

// Reserves memory up to desired count. Calls 'Realloc' internally.
template <typename T>
void array_reserve(Array<T> *arr, u64 desired_count);

// Sets the count field to 0.
template <typename T>
void array_reset_keeping_memory(Array<T> *arr);

// Sets the count field to 0, and frees memory.
template <typename T>
void array_reset(Array<T> *arr);




template<typename T>
T& Array <T>::operator [] (u64 idx) {
    return data[idx];
}

template<typename T>
const T& Array <T>::operator [] (u64 idx) const {
    return data[idx];
}

template <typename T>
void array_add(Array<T>* arr, T item) {
    if (arr->count >= arr->allocated) {
        u64 reserve = max(8ull, 2 * arr->count);
        array_reserve(arr, reserve);
    }
    arr->data[arr->count] = item;
    arr->count += 1;
}

template <typename T>
void array_reserve(Array<T>* arr, u64 desired_count) {
    if (desired_count <= arr->allocated) return;

    if (!arr->allocator.proc) {
        arr->allocator = tctx.allocator;
    }

    arr->data = (T *)realloc(arr->data, desired_count * sizeof(T), arr->allocated * sizeof(T), arr->allocator);
    Assert(arr->data != NULL);

    arr->allocated = desired_count;
}

template <typename T>
void array_reset_keeping_memory(Array<T>* arr) {
    arr->count = 0;
}

template <typename T>
void array_reset(Array<T> *arr) {
    dealloc(arr->data, arr->allocator);
    arr->data = NULL;
    arr->count = 0;
}


#endif // RTS_ARRAY_H
