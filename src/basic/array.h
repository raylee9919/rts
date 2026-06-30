// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RTS_ARRAY_H
#define RTS_ARRAY_H


template <typename T>
struct Array {
    T*   data       = nullptr;
    u64  count      = 0;
    u64  allocated  = 0;

    Allocator allocator = {};

    T& operator[](u64 idx);
    const T& operator[](u64 idx) const;
};

template <typename T>
internal void array_add(Array<T>* arr, T item);

template <typename T>
internal void array_reserve(Array<T>* arr, u64 desired_count);

template <typename T>
internal void array_reset_keeping_memory(Array<T>* arr);

template <typename T>
internal void array_reset(Array<T>* arr);




#endif // RTS_ARRAY_H
