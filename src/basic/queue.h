// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RTS_QUEUE_H
#define RTS_QUEUE_H

#include "basic/core.h"
#include "basic/array.h"

template <typename T>
struct Queue {
    u64 front_index = 0;
    u64 rear_index  = 0;
    u64 count       = 0;
    u64 allocated   = 0;
    Array<T> array  = {};
};

template <typename T>
T queue_front(Queue<T> *q);

template <typename T>
void queue_push(Queue<T> *q, T item);

template <typename T>
void queue_pop(Queue<T> *q);

template <typename T>
void queue_reserve(Queue<T> *q, u64 count);



// @Correctness

template <typename T>
T queue_front(Queue<T> *q)
{
    R_ASSERT(q->count > 0);

    return q->array.data[q->front_index];
}

template <typename T>
void queue_push(Queue<T> *q, T item)
{
    if (q->count == q->allocated) {
        u64 new_allocated = max(8ull, 2 * q->allocated);
        queue_reserve(q, new_allocated);
    }

    q->array.data[q->rear_index] = item;
    q->rear_index = (q->rear_index + 1) % q->allocated;
    q->count += 1;
}

template <typename T>
void queue_pop(Queue<T> *q)
{
    R_ASSERT(q->count > 0);

    q->front_index = (q->front_index + 1) % q->allocated;
    q->count -= 1;
}

template <typename T>
void queue_reserve(Queue<T> *q, u64 N)
{
    u64 old_allocated = q->allocated;
    if (N <= old_allocated)  return;

    array_reserve(&q->array, N);
    u64 new_allocated = q->array.allocated;

    if (q->front_index + q->count > old_allocated) {
        u64 tail      = old_allocated - q->front_index;
        u64 new_front = new_allocated - tail;
        memmove(q->array.data + new_front, q->array.data + q->front_index, tail * sizeof(T));
        q->front_index = new_front;
    }

    q->allocated  = new_allocated;
    q->rear_index = (q->front_index + q->count) % new_allocated;
}


#endif // RTS_QUEUE_H
