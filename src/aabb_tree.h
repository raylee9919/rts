// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RTS_AABB_TREE_H
#define RTS_AABB_TREE_H

#include "basic/arena.h"
#include "basic/array.h"
#include "math/math.h"
#include "handle.h"

struct Box_Node
{
  // [size, align]
  AABBV box;     // [32,16] 
  s32   left;    // [ 4, 4]  index of the left  child node. -1 if none.
  s32   right;   // [ 4, 4]  index of the right child node. -1 if none.
  s32   proxy;   // [ 4, 4]  index of the proxy. Only meaningful when the node is a leaf.
  s32   parent;  // [ 4, 4]  index of the parent node. -1 if the node is the root.
  s32   next;    // [ 4, 4]  index of the next free node in the free list.
};

struct Box_Proxy
{
  Handle item;
  s32    next; // next free proxy index. -1 if none.
  s32    node; // index of the node that owns this proxy.
};

struct Box_Tree
{
  Arena *arena;

  Box_Node *nodes;
  s32 num_nodes;
  s32 cap_nodes;
  s32 free_node;

  Box_Proxy *proxies;
  s32 num_proxies;
  s32 cap_proxies;
  s32 free_proxy;

  s32 root; // index of the root node. -1 if empty.
};


void 
aabb_tree_init(Box_Tree *tree, Arena *arena);

s32 
aabb_tree_insert(Box_Tree *tree, Handle item, AABBV box);

void 
aabb_tree_remove(Box_Tree *tree, s32 proxy_index);

void
aabb_tree_move(Box_Tree *tree, s32 proxy_index, AABBV box);

Array<Handle>
aabb_tree_query(Box_Tree *tree, m4x4 view_frustum, Allocator allocator);

void 
aabb_tree_visualize(Box_Tree *tree);



#endif // RTS_AABB_TREE_H
