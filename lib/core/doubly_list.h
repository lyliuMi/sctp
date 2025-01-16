#ifndef _DOUBLELY_LIST_H
#define _DOUBLELY_LIST_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C"{
#endif

struct doubly_list_s
{
    struct doubly_list_s *prev;
    struct doubly_list_s *next;
};

typedef struct doubly_list_s doubly_list_t;
typedef struct doubly_list_s lnode_t;

#define DOUBLY_LIST(name) \
    doubly_list_t name = {NULL , NULL}

#define DOUBLY_LIST_INIT(list) do{ \
    (list)->prev = (NULL); \
    (list)->next = (NULL); \
}while(0)

static inline void *doubly_list_first(const doubly_list_t *list)
{
    return list->next;
}

static inline void *doubly_list_last(const doubly_list_t *list)
{
    return list->prev;
}

static inline void *doubly_list_next(void *lnode)
{
    doubly_list_t *node = (doubly_list_t *)lnode;
    return node->next;
}

static inline void *doubly_list_prev(void *lnode)
{
    doubly_list_t *node = (doubly_list_t *)lnode;
    return node->prev;
}

#define doubly_list_for_each(list, node) \
    for (node = doubly_list_first(list); (node); \
        node = doubly_list_next(node))

#define doubly_list_reverse_for_each(list, node) \
    for (node = doubly_list_last(list); (node); \
        node = doubly_list_prev(node))


#define doubly_list_for_each_safe(list, n, node) \
    for (node = doubly_list_first(list); \
        (node) && (n = doubly_list_next(node), 1); \
        node = n)

typedef int (*doubly_list_compare_f)(lnode_t *n1, lnode_t *n2);
#define doubly_list_insert_sorted(__list, __lnode, __compare) \
    __doubly_list_insert_sorted(__list, __lnode, (doubly_list_compare_f)__compare);

static inline bool doubly_list_empty(const doubly_list_t *list)
{
    return list->next == NULL;
}

void doubly_list_prepend(doubly_list_t *list, void *lnode);
void doubly_list_add(doubly_list_t *list, void *lnode);
void doubly_list_remove(doubly_list_t *list, void *lnode);
void doubly_list_insert_prev(doubly_list_t *list, void *lnext, void *lnode);
void doubly_list_insert_next(doubly_list_t *list, void *lprev, void *lnode);
void __doubly_list_insert_sorted(doubly_list_t *list, void *lnode, doubly_list_compare_f compare);
int doubly_list_count(const doubly_list_t *list);
bool doubly_list_exists(const doubly_list_t *list, void *lnode);


#ifdef __cplusplus
}
#endif

#endif