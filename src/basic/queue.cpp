// Copyright Seong Woo Lee. All Rights Reserved.


// @Correctness

template <typename T>
internal T queue_front(Queue<T> *q)
{
    Assert(q->count > 0);

    return q->array.data[q->front_index];
}

template <typename T>
internal void queue_push(Queue<T> *q, T item)
{
    if (q->count == q->allocated)
    {
        u64 old_count = q->count;
        u64 new_allocated = max(8ull, 2 * q->allocated);

        array_reserve(&q->array, new_allocated);

        // Linearize the queue.
        for (u64 i = 0; i < old_count; ++i)
        {
            q->array.data[i] =
                q->array.data[(q->front_index + i) % q->allocated];
        }

        q->front_index = 0;
        q->rear_index  = old_count;
        q->allocated   = new_allocated;
    }

    q->array.data[q->rear_index] = item;
    q->rear_index = (q->rear_index + 1) % q->allocated;
    q->count += 1;
}

template <typename T>
internal void queue_pop(Queue<T> *q)
{
    Assert(q->count > 0);

    q->front_index = (q->front_index + 1) % q->allocated;
    q->count -= 1;
}
