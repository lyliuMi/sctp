#ifndef _L_RBTREE_H
#define _L_RBTREE_H

#include <stdio.h>
#include <assert.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif


typedef enum 
{
    RBTREE_BLACK = 0,
    RBTREE_RED = 1,
}rbtree_color_e;

typedef struct rbnode_s 
{
    struct rbnode_s *parent;
    struct rbnode_s *left;
    struct rbnode_s *right;

    rbtree_color_e color;
}rbnode_t;

typedef struct rbtree_s 
{
    rbnode_t *root;
}rbtree_t;

#define RBTREE(name) rbtree_t name = { NULL }

// #define ogs_rb_entry(ptr, type, member) ogs_container_of(ptr, type, member)

static inline void rbtree_link_node(
        void *rb_node, rbnode_t *parent, rbnode_t **rb_link)
{
    rbnode_t *node = (rbnode_t *)rb_node;
    node->parent = parent;
    node->left = node->right = NULL;
    node->color = RBTREE_RED;

    *rb_link = node;
}

void rbtree_insert_color(rbtree_t *tree, void *rb_node);
void rbtree_delete(rbtree_t *tree, void *rb_node);

static inline void *rbtree_min(const rbnode_t *rb_node)
{
    const rbnode_t *node = rb_node;
    assert(node);

    while (node->left)
        node = node->left;

    return (void *)node;
}

static inline void *rbtree_max(const void *rb_node)
{
    const rbnode_t *node = (const rbnode_t *)rb_node;
    assert(node);

    while (node->right)
        node = node->right;

    return (void *)node;
}

void *rbtree_first(const rbtree_t *tree);
void *rbtree_next(const void *node);
void *rbtree_last(const rbtree_t *tree);
void *rbtree_prev(const void *node);

#define rbtree_for_each(tree, node) \
    for (node = rbtree_first(tree); \
        (node); node = rbtree_next(node))

#define rbtree_reverse_for_each(tree, node) \
    for (node = rbtree_last(tree); \
        (node); node = rbtree_prev(node))

static inline bool rbtree_empty(const rbtree_t *tree)
{
    return tree->root == NULL;
}

static inline int rbtree_count(const rbtree_t *tree)
{
    void *node;
    int i = 0;
    rbtree_for_each(tree, node)
        i++;
    return i;
}

#ifdef __cplusplus
}
#endif

#endif /* OGS_RBTREE_H */
