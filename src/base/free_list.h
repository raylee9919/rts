// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RTS_FREE_LIST_H
#define RTS_FREE_LIST_H

struct Free_List_Node {
    Free_List_Node* next;
    Free_List_Node* prev;
    u64             slots;
};

struct Free_List {
    Free_List_Node* first;
    Free_List_Node* last;
};

#endif // RTS_FREE_LIST_H
