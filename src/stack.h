/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Seong Woo Lee $
   $Notice: (C) Copyright 2025 by Seong Woo Lee. All Rights Reserved. $
   ======================================================================== */



    
template <typename T>
struct Stack {
    T data[256];
    s32 top = -1;
};

template <typename T>
static T pop(Stack<T> *stk) {
    Assert(stk->top > -1);
    return stk->data[stk->top--];
}

template <typename T>
static void push(Stack<T> *stk, T item) {
    Assert(stk->top + 1 < arraycount(stk->data));
    stk->data[++stk->top] = item;
}

template <typename T>
static void clear(Stack<T> *stk) {
    stk->top = -1;
}

template <typename T>
static bool empty(Stack<T> *stk) {
    if (stk->top == -1) return true;
    return false;
}
