// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RTS_PRIORITY_QUEUE_H
#define RTS_PRIORITY_QUEUE_H

#include "basic/core.h"
#include "basic/allocator.h"

template<typename T>
struct Priority_Queue
{
  T   *data      = nullptr;
  u64  count     = 0;
  u64  allocated = 0;

  Allocator allocator = {};
};

template<typename T>
void heapify_up(Priority_Queue<T> *pq, u64 index)
{
  while (index > 0)
  {
    u64 parent = (index - 1) / 2;
    if (!(pq->data[index] < pq->data[parent])) break;
    swap(&pq->data[index], &pq->data[parent]);
    index = parent;
  }
}

template<typename T>
void heapify_down(Priority_Queue<T> *pq, u64 index)
{
  for (;;)
  {
    u64 smallest = index;
    u64 left     = 2 * index + 1;
    u64 right    = 2 * index + 2;

    if (left  < pq->count && pq->data[left]  < pq->data[smallest]) smallest = left;
    if (right < pq->count && pq->data[right] < pq->data[smallest]) smallest = right;
    if (smallest == index) break;

    swap(&pq->data[index], &pq->data[smallest]);
    index = smallest;
  }
}

template<typename T>
void priority_queue_reserve(Priority_Queue<T> *pq, u64 desired_count)
{
  if (desired_count <= pq->allocated) return;
  R_ASSERT(pq->allocator.proc);

  pq->data = (T *)realloc(pq->data, desired_count * sizeof(T), pq->allocated * sizeof(T), pq->allocator);
  R_ASSERT(pq->data != NULL);
  pq->allocated = desired_count;
}

template<typename T>
void priority_queue_push(Priority_Queue<T> *pq, T item)
{
  if (pq->count >= pq->allocated)
  {
    priority_queue_reserve(pq, max(8ull, 2 * pq->allocated));
  }

  pq->data[pq->count] = item;
  pq->count += 1;

  heapify_up(pq, pq->count - 1);
}

template<typename T>
T priority_queue_pop(Priority_Queue<T> *pq)
{
  R_ASSERT(pq->count > 0);

  T item = pq->data[0];
  pq->count -= 1;
  pq->data[0] = pq->data[pq->count];
  heapify_down(pq, 0);
  return item;
}

#endif // RTS_PRIORITY_QUEUE_H
