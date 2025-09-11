/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Sung Woo Lee $
   $Notice: (C) Copyright 2025 by Sung Woo Lee. All Rights Reserved. $
   ======================================================================== */

template<typename X, typename Y>
struct Pair {
    union {
        struct {
            X x;
            Y y;
        };
        struct {
            X first;
            Y second;
        };
    };
};

template<typename X, typename Y>
bool operator > (Pair<X, Y> a, Pair<X, Y> b) 
{
    if (a.x > b.x) return true;
    return false;
}

template<typename X, typename Y>
bool operator < (Pair<X, Y> a, Pair<X, Y> b) 
{
    if (a.x < b.x) return true;
    return false;
}

template<typename T>
struct Priority_Queue {
    T items[256];
    size_t size;
};

template<typename T>
void swap(T *a, T *b) {
    T tmp = *a;
    *a = *b;
    *b = tmp;
}

template<typename T>
void heapifyUp(Priority_Queue<T> *pq, size_t index)
{
    if (index
        && pq->items[(index - 1) / 2] > pq->items[index]) {
        swap(&pq->items[(index - 1) / 2],
             &pq->items[index]);
        heapifyUp(pq, (index - 1) / 2);
    }
}

template<typename T>
void enqueue(Priority_Queue<T> *pq, T value)
{
    if (pq->size < array_count(pq->items)) {
        pq->items[pq->size++] = value;
        heapifyUp(pq, pq->size - 1);
    } else {
        INVALID_CODE_PATH;
    }
}

template<typename T>
void heapifyDown(Priority_Queue <T> *pq, size_t index)
{
    size_t smallest = index;
    size_t left = 2 * index + 1;
    size_t right = 2 * index + 2;

    if (left < pq->size
        && pq->items[left] < pq->items[smallest])
        smallest = left;

    if (right < pq->size
        && pq->items[right] < pq->items[smallest])
        smallest = right;

    if (smallest != index) {
        swap(&pq->items[index], &pq->items[smallest]);
        heapifyDown(pq, smallest);
    }
}

template<typename T>
T dequeue(Priority_Queue<T> *pq)
{
    if (pq->size) {
        T item = pq->items[0];
        pq->items[0] = pq->items[--pq->size];
        heapifyDown(pq, 0);
        return item;
    } else {
        INVALID_CODE_PATH;
        return {};
    }
}

template<typename T>
T peek(Priority_Queue<T> *pq)
{
    if (pq->size) {
        return pq->items[0];
    }
}
