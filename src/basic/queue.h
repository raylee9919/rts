// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RTS_QUEUE_H
#define RTS_QUEUE_H

template <typename T>
struct Queue {
    u64 front_index = 0;
    u64 rear_index  = 0;
    u64 count       = 0;
    u64 allocated   = 0;
    Array<T> array;
};

template <typename T>
internal T queue_front(Queue<T> *q);

template <typename T>
internal void queue_push(Queue<T> *q, T item);

template <typename T>
internal void queue_pop(Queue<T> *q);


#endif // RTS_QUEUE_H
