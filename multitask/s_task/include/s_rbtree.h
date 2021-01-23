/*-------------------------------------------------------------------------
 *
 * rbtree.h
 *      interface for PostgreSQL generic Red-Black binary tree package
 *
 * Copyright (c) 2009-2020, PostgreSQL Global Development Group
 *
 * IDENTIFICATION
 *      src/include/lib/rbtree.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef INC_S_RBTREE_H_
#define INC_S_RBTREE_H_

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif


#ifndef GET_PARENT_ADDR
#define GET_PARENT_ADDR(pMe,tParent,eMyName) \
    ((tParent *)((char *)(pMe) - (ptrdiff_t)&((tParent *)0)->eMyName))
#endif


/*
 * RBTNode is intended to be used as the first field of a larger struct,
 * whose additional fields carry whatever payload data the caller needs
 * for a tree entry.  (The total size of that larger struct is passed to
 * rbt_create.) RBTNode is declared here to support this usage, but
 * callers must treat it as an opaque struct.
 */
typedef struct RBTNode
{
    char color;                 /* node's current color, red or black */
    struct RBTNode *left;       /* left child, or RBTNIL if none */
    struct RBTNode *right;      /* right child, or RBTNIL if none */
    struct RBTNode *parent;     /* parent, or NULL (not RBTNIL!) if none */
} RBTNode;


#define RBTNIL (&g_sentinel)
extern RBTNode g_sentinel;

/* Support functions to be provided by caller */
typedef int (*rbt_comparator) (const RBTNode *a, const RBTNode *b, void *arg);
typedef int (*rbt_find_comparator) (const void *a, const RBTNode *b);
/* 
typedef void (*rbt_combiner) (RBTNode *existing, const RBTNode *newdata, void *arg);
typedef RBTNode *(*rbt_allocfunc) (void *arg);
typedef void (*rbt_freefunc) (RBTNode *x, void *arg);
*/

/*
 * RBTree control structure
 */
typedef struct
{
    RBTNode *root;          /* root node, or RBTNIL if tree is empty */

    /* Remaining fields are constant after rbt_create */
    rbt_comparator comparator;
    void* comparator_arg;
} RBTree;

/* Available tree iteration orderings */
typedef enum RBTOrderControl
{
    LeftRightWalk,              /* inorder: left child, node, right child */
    RightLeftWalk               /* reverse inorder: right, node, left */
} RBTOrderControl;

/*
 * RBTreeIterator holds state while traversing a tree.  This is declared
 * here so that callers can stack-allocate this, but must otherwise be
 * treated as an opaque struct.
 */
typedef struct RBTreeIterator RBTreeIterator;

struct RBTreeIterator
{
    RBTree     *rbt;
    RBTNode    *(*iterate) (RBTreeIterator *iter);
    RBTNode    *last_visited;
    bool        is_over;
};


extern RBTree *rbt_create(RBTree* tree,
                          rbt_comparator comparator,
                          void *comparator_arg);

extern RBTNode *rbt_find(RBTree *rbt, const RBTNode *data);
extern RBTNode* rbt_find2(RBTree* rbt, rbt_find_comparator comparator, const void* data);
extern RBTNode *rbt_leftmost(RBTree *rbt);

extern bool rbt_insert(RBTree *rbt, RBTNode *data);
extern void rbt_delete(RBTree *rbt, RBTNode *node);

extern void rbt_begin_iterate(RBTree *rbt, RBTOrderControl ctrl,
                              RBTreeIterator *iter);
extern RBTNode *rbt_iterate(RBTreeIterator *iter);
extern bool rbt_is_empty(const RBTree* rbt);

#ifdef __cplusplus
}
#endif
#endif /* INC_S_RBTREE_H_ */
