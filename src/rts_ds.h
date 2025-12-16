/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Seong Woo Lee $
   $Notice: (C) Copyright 2025 by Seong Woo Lee. All Rights Reserved. $
   ======================================================================== */

template<typename T>
struct Array {
    Array();
    ~Array();

    void push(T val);
    void clear();
    bool empty();

    T &operator[](u64 idx);
    const T& operator[](u64 idx) const;

    T *data = 0;
    int count = 0;
    int cap = 0;
};

template<typename T>
struct Stack {
    Stack();
    ~Stack();

    void push(T val);
    T    pop();
    int  count();
    bool empty();
    void clear();

    T data[256];
    int top = 0;
};

template<typename T>
struct Queue {
    Queue();
    ~Queue();

    void push(T val);
    T    pop();
    int  count();
    bool empty();
    void clear();
    T    front();

    T data[256];
    int front_idx = 0;
    int back_idx  = 0;
};


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
bool operator > (Pair<X, Y> a, Pair<X, Y> b) {
    if (a.x > b.x) return true;
    return false;
}

template<typename X, typename Y>
bool operator < (Pair<X, Y> a, Pair<X, Y> b) {
    if (a.x < b.x) return true;
    return false;
}

template<typename T>
struct Priority_Queue {
    T items[256];
    u64 size;

    void push(T value);
    T pop();
};

template<typename T>
void swap(T *a, T *b) {
    T tmp = *a;
    *a = *b;
    *b = tmp;
}

template<typename T>
void heapifyUp(Priority_Queue<T> *pq, size_t index) {
    if (index
        && pq->items[(index - 1) / 2] > pq->items[index]) {
        swap(&pq->items[(index - 1) / 2],
             &pq->items[index]);
        heapifyUp(pq, (index - 1) / 2);
    }
}

template<typename T>
void heapifyDown(Priority_Queue <T> *pq, size_t index)
{
    size_t smallest = index;
    size_t left = 2 * index + 1;
    size_t right = 2 * index + 2;

    if (left < pq->size && pq->items[left] < pq->items[smallest])
    {
        smallest = left;
    }

    if (right < pq->size && pq->items[right] < pq->items[smallest])
    {
        smallest = right;
    }

    if (smallest != index) 
    {
        swap(&pq->items[index], &pq->items[smallest]);
        heapifyDown(pq, smallest);
    }
}
