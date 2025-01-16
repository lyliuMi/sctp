#include "doubly_list.h"
#include <stdio.h>

void doubly_list_prepend(doubly_list_t *list, void *lnode)
{
    doubly_list_t *node = (doubly_list_t *)lnode;

    node->prev = NULL;
    node->next = list->next;
    if (list->next)
        list->next->prev = node;
    else
        list->prev = node;
    list->next = node;
}

void doubly_list_add(doubly_list_t *list, void *lnode)
{
    doubly_list_t *node = (doubly_list_t *)lnode;

    node->prev = list->prev;
    node->next = NULL;
    if (list->prev)
        list->prev->next = node;
    else
        list->next = node;
    list->prev = node;
}

void doubly_list_remove(doubly_list_t *list, void *lnode)
{
    doubly_list_t *node = (doubly_list_t *)lnode;
    doubly_list_t *prev = node->prev;
    doubly_list_t *next = node->next;

    if (prev)
        prev->next = next;
    else
        list->next = next;

    if (next)
        next->prev = prev;
    else
        list->prev = prev;
}

void doubly_list_insert_prev(doubly_list_t *list, void *lnext, void *lnode)
{
    doubly_list_t *node = (doubly_list_t *)lnode;
    doubly_list_t *next = (doubly_list_t *)lnext;

    node->prev = next->prev;
    node->next = next;
    if (next->prev)
        next->prev->next = node;
    else
        list->next = node;
    next->prev = node;
}

void doubly_list_insert_next(doubly_list_t *list, void *lprev, void *lnode)
{
    doubly_list_t *node = (doubly_list_t *)lnode;
    doubly_list_t *prev = (doubly_list_t *)lprev;

    node->prev = prev;
    node->next = prev->next;
    if (prev->next)
        prev->next->prev = node;
    else
        list->prev = node;
    prev->next = node;
}



void __doubly_list_insert_sorted(doubly_list_t *list, void *lnode, doubly_list_compare_f compare)
{
    doubly_list_t *node = (doubly_list_t *)lnode;
    void *iter = NULL;

    doubly_list_for_each(list, iter) {
        if ((*compare)(node, (doubly_list_t *)iter) < 0) {
            doubly_list_insert_prev(list, iter, node);
            break;
        }
    }

    if (iter == NULL)
        doubly_list_add(list, node);
}


int doubly_list_count(const doubly_list_t *list)
{
    void *node;
    int i = 0;
    doubly_list_for_each(list, node)
        i++;
    return i;
}

bool doubly_list_exists(const doubly_list_t *list, void *lnode)
{
    doubly_list_t *node = (doubly_list_t *)lnode;
    void *iter = NULL;

    doubly_list_for_each(list, iter) {
        if (node == (doubly_list_t *)iter)
            return true;
    }

    return false;
}