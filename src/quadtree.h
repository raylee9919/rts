// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RTS_QUADTREE_H
#define RTS_QUADTREE_H

#include "basic/array.h"
#include "math/math.h"
#include "handle.h"

struct Game_State;
struct Camera;


#define QUADTREE_ITEM_THRESHOLD 4
#define QUADTREE_MAX_DEPTH      8


struct Quadtree_Item {
  Handle item;
  Rect   rect;
};

struct Quadtree_Node {
  s32           index;             // self index into 'tree.nodes'
  s32           first_child_index; // index into 'tree.nodes'. '-1' if the node is a leaf.
  s32           depth;             // '0' for root.
  s32           num_items;
  s32           item_index;        // index into 'tree.items'. '-1' if it has no item (but why?)
  Rect          rect;
};

struct Quadtree {
  Array<Quadtree_Node> nodes;
  Array<Quadtree_Item> items;
};

void 
quadtree_build(Game_State  *g, 
               Quadtree    *tree, 
               vec2         min, 
               vec2         max);

void 
quadtree_visualize(Quadtree *tree, 
                   Camera   *camera);


#endif // RTS_QUADTREE_H
