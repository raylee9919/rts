// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RTS_ARRAY_H
#define RTS_ARRAY_H

#include "basic/core.h"
#include "basic/allocator.h"
#include "basic/context.h"

template <typename Type>
struct Array 
{
  Type *data      = nullptr;
  u64  count      = 0;
  u64  allocated  = 0;

  Allocator allocator = {};

  Array() = default;
  Array(Allocator allocator_) : allocator(allocator_) {}
  Array(Allocator allocator_, u64 n) : allocator(allocator_) {
    array_reserve(this, n);
  }

  Type& operator [] (u64 idx);
  const Type& operator [] (u64 idx) const;

  Type *begin() { return data;         }
  Type *end()   { return data + count; }
};


template<typename T>
T& Array <T>::operator [] (u64 idx) 
{
  return data[idx];
}

template<typename T>
const T& Array <T>::operator [] (u64 idx) const 
{
  return data[idx];
}

template <typename T>
void 
append(Array<T>* arr, T item) 
{
  if (arr->count >= arr->allocated) {
    u64 reserve = max(8ull, 2 * arr->count);
    array_reserve(arr, reserve);
  }
  arr->data[arr->count] = item;
  arr->count += 1;
}

template <typename T> 
b32 
append_unique(Array<T> *arr, T item) 
{
  for (u64 i = 0; i < arr->count; ++i) {
    if (arr->data[i] == item) return false;
  }

  append(arr, item);
  return true;
}

template <typename T> 
void 
array_reserve(Array<T>* arr, u64 desired_count) 
{
  if (desired_count <= arr->allocated) return;

  if (!arr->allocator.proc) {
    R_ASSERT(0);
  }

  arr->data = (T *)realloc(arr->data, desired_count * sizeof(T), arr->allocated * sizeof(T), arr->allocator);
  R_ASSERT(arr->data != NULL);

  arr->allocated = desired_count;
}

template <typename T>
void 
array_reset_keeping_memory(Array<T>* arr) 
{
  arr->count = 0;
}

template <typename T>
void 
array_reset(Array<T> *arr) 
{
  dealloc(arr->data, arr->allocator);
  arr->data = NULL;
  arr->count = 0;
}

template <typename T>
void 
array_pop_back(Array<T> *arr)
{
  R_ASSERT(arr->count > 0);
  arr->count -= 1;
}

// Returns index of the first occurence of `item` in the array.
// Returns -1 if the item doesn't exist.
template <typename T> 
s64
array_find(Array<T> *arr, T item, b32 (*cmp)(T x, T y))
{
  for (s64 i = 0; i < arr->count; ++i)
  {
    if (cmp(arr->data[i], item))
    {
      return i;
    }
  }
  return -1;
}


//
// S t a c k
//
template <typename T>
T
stack_top(Array<T> *arr)
{
  R_ASSERT(arr->count > 0);
  return arr->data[arr->count - 1];
}

template <typename T>
void
stack_pop(Array<T> *arr)
{
  R_ASSERT(arr->count > 0);
  array_pop_back(arr);
}

template <typename T>
void
stack_push(Array<T> *arr, T item)
{
  append(arr, item);
}


#endif // RTS_ARRAY_H
