#ifndef _L_TIMER_H
#define _L_TIMER_H

#include "l_rbtree.h"
#include "doubly_list.h"
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct timer_mgr_s timer_mgr_t;
typedef struct my_timer_s 
{
    rbnode_t rbnode;
    lnode_t lnode;

    void (*cb)(void*);
    void *data;

    timer_mgr_t *manager;
    bool running;
    time_t timeout;
}my_timer_t;

timer_mgr_t *timer_mgr_create(unsigned int capacity);
void timer_mgr_destroy(timer_mgr_t *manager);

my_timer_t *timer_add(
        timer_mgr_t *manager, void (*cb)(void *data), void *data);
#define timer_delete(timer) \
    timer_delete_debug(timer, __FILE__)
void timer_delete_debug(my_timer_t *timer, const char *file_line);

#define timer_start(timer, duration) \
    timer_start_debug(timer, duration, __FILE__)
void timer_start_debug(
        my_timer_t *timer, time_t duration, const char *file_line);
#define timer_stop(timer) \
    timer_stop_debug(timer, __FILE__)
void timer_stop_debug(my_timer_t *timer, const char *file_line);

time_t timer_mgr_next(timer_mgr_t *manager);
void timer_mgr_expire(timer_mgr_t *manager);

#ifdef __cplusplus
}
#endif

#endif /* OGS_TIMER_H */
