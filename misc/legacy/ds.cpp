// Copyright Seong Woo Lee. All Rights Reserved.


//
// List
//
template <typename T>
bool List <T> :: is_empty() {
    if (first == nullptr || last == nullptr) {
        assert(first == last);
        return true;
    }
    return false;
}

template <typename T>
void List <T> :: add(T item) {
    auto* node = new Link <T>; // @Todo: Remove this shii
    memset(node, 0, sizeof(*node));
    
    node->data = item;

    dll_push_back(first, last, node);
}

template <typename T>
void List <T> :: clear() {
    for (Link <T>* node = first, *next; node; node = next) {
        next = node->next;
        
        dll_remove(first, last, node);
        delete node;
    }
}



//
// Queue
//
template<typename T>
void Queue<T>::push(T val) {
    int next = (back_idx + 1)%array_count(data);
    assert(next != front_idx);
    data[back_idx] = val;
    back_idx = next;
}

template<typename T>
T Queue<T>::pop() {
    assert(front_idx != back_idx);
    T val = data[front_idx];
    front_idx = (front_idx + 1) % array_count(data);
    return val;
}

template<typename T>
int Queue<T>::count() {
    if (back_idx >= front_idx) {
        return back_idx - front_idx;
    } else {
        return back_idx + array_count(data) - front_idx;
    }
}

template<typename T>
T Queue<T>::front() {
    assert(front_idx != back_idx);
    return data[front_idx];
}

template<typename T>
bool Queue<T>::empty() {
    return front_idx == back_idx;
}

template<typename T>
void Queue<T>::clear() {
    front_idx = 0;
    back_idx = 0;
}


//
// Priority_Queue
//
template<typename T>
void Priority_Queue<T>::push(T value) {
    if (size < array_count(items)) {
        items[size++] = value;
        heapifyUp(this, size - 1);
    } else {
        INVALID_CODE_PATH;
    }
}

template<typename T>
T Priority_Queue<T>::pop() {
    if (size) {
        T item = items[0];
        items[0] = items[--size];
        heapifyDown(this, 0);
        return item;
    } else {
        INVALID_CODE_PATH;
        return {};
    }
}
