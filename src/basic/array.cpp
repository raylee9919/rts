// Copyright Seong Woo Lee. All Rights Reserved.

template<typename T>
T& Array <T>::operator[](u64 idx) {
    return data[idx];
}

template<typename T>
const T& Array <T>::operator[](u64 idx) const {
    return data[idx];
}

template <typename T>
void array_add(Array<T>* arr, T item) {
    if (arr->count >= arr->allocated) {
        u64 reserve = max(8, 2 * arr->count);
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

    arr->data = (T*)realloc(arr->data, desired_count * sizeof(T), arr->allocated * sizeof(T));
    assert(arr->data != nullptr);

    arr->allocated = desired_count;
}

template <typename T>
void array_reset_keeping_memory(Array<T>* arr) {
    arr->count = 0;
}

template <typename T>
void array_reset(Array<T>* arr) {
    Free(arr->date);
    arr->data = nullptr;
    arr->count = 0;
}
