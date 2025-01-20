#include "l_timer.h"
#include "mempool.h"
#include <sys/time.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>

typedef struct timer_mgr_s {
    MEM_POOL(pool, my_timer_t);
    rbtree_t tree;
}timer_mgr_t;

#define USEC_PER_SEC (1000000LL)
#define time_from_sec(sec) ((time_t)(sec) * USEC_PER_SEC)
time_t get_monotonic_time(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return time_from_sec(tv.tv_sec) + tv.tv_usec;
}

static void add_timer_node(
        rbtree_t *tree, my_timer_t *timer, time_t duration)
{
    rbnode_t **new = NULL;
    rbnode_t *parent = NULL;
    assert(tree);
    assert(timer);

    timer->timeout = get_monotonic_time() + duration;

    new = &tree->root;
    while (*new) {
        my_timer_t *this = (my_timer_t *)*new;

        parent = *new;
        if (timer->timeout < this->timeout)
            new = &(*new)->left;
        else
            new = &(*new)->right;
    }

    rbtree_link_node(timer, parent, new);
    rbtree_insert_color(tree, timer);
}

timer_mgr_t *timer_mgr_create(unsigned int capacity)
{
    timer_mgr_t *manager = calloc(1, sizeof *manager);
    if (!manager) {
        fprintf(stderr, "ogs_calloc() failed");
        return NULL;
    }

    mempool_init(&manager->pool, capacity);

    return manager;
}

void timer_mgr_destroy(timer_mgr_t *manager)
{
    assert(manager);

    mempool_destroy(&manager->pool);
    free(manager);
}

my_timer_t *timer_add(
        timer_mgr_t *manager, void (*cb)(void *data), void *data)
{
    my_timer_t *timer = NULL;
    assert(manager);

    mempool_alloc(&manager->pool, &timer);
    if (!timer) {
        fprintf(stderr, "ogs_pool_alloc() failed");
        return NULL;
    }

    memset(timer, 0, sizeof *timer);
    timer->cb = cb;
    timer->data = data;

    timer->manager = manager;

    return timer;
}

void timer_delete_debug(my_timer_t *timer, const char *file_line)
{
    timer_mgr_t *manager;
    assert(timer);
    manager = timer->manager;
    assert(manager);

    timer_stop(timer);

    mempool_free(&manager->pool, timer);
}

void timer_start_debug(
        my_timer_t *timer, time_t duration, const char *file_line)
{
    timer_mgr_t *manager = NULL;
    assert(timer);
    assert(duration);

    manager = timer->manager;
    assert(manager);

    if (timer->running == true)
        rbtree_delete(&manager->tree, timer);

    timer->running = true;
    add_timer_node(&manager->tree, timer, duration);
}

void timer_stop_debug(my_timer_t *timer, const char *file_line)
{
    timer_mgr_t *manager = NULL;
    assert(timer);
    manager = timer->manager;
    assert(manager);

    if (timer->running == false)
        return;

    timer->running = false;
    rbtree_delete(&manager->tree, timer);
}

time_t timer_mgr_next(timer_mgr_t *manager)
{
    time_t current;
    rbnode_t *rbnode = NULL;
    assert(manager);

    current = get_monotonic_time();
    rbnode = rbtree_first(&manager->tree);
    if (rbnode) {
        my_timer_t *this = (my_timer_t *)rbnode;
        if (this->timeout > current) {
            return (this->timeout - current);
        } else {
            return 0;
        }
    }

    return -1;
}

void timer_mgr_expire(timer_mgr_t *manager)
{
    DOUBLY_LIST(list);
    lnode_t *lnode;

    time_t current;
    rbnode_t *rbnode;
    my_timer_t *this;
    assert(manager);

    current = get_monotonic_time();

    rbtree_for_each(&manager->tree, rbnode) {
        this = (my_timer_t *)rbnode;

        if (this->timeout > current)
            break;

        doubly_list_add(&list, &this->lnode);
    }

    /* You should not perform a delete on a timer using ogs_timer_delete()
     * in a callback function this->cb(). */
    doubly_list_for_each(&list, lnode) {
        this = (my_timer_t *)((unsigned char*)lnode-sizeof(rbnode_t));
        timer_stop(this);
        if (this->cb)
            this->cb(this->data);
    }
}
