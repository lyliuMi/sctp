#ifndef _POLLBOX_H
#define _POLLBOX_H

#include <sys/select.h>
#include <sys/epoll.h>
#include "mempool.h"
#include "doubly_list.h"


#ifdef __cplusplus
extern "C" {
#endif

#define _EPOLLIN_      0x01
#define _EPOLLOUT_     0x02

typedef void (*poll_handler_f)(short when, int fd, void *data);

typedef struct poll_s poll_t;
typedef struct pollset_s
{
    void *context;
    MEM_POOL(pool, poll_t);
    unsigned int capacity;
}pollset_t;

typedef struct poll_s 
{
    lnode_t node;
    int index;

    short when;
    int fd;
    poll_handler_f handler;
    void *data;
    pollset_t *pollset;
}poll_t;



typedef struct pollset_actions_s
{
    void (*init)(pollset_t *pollset);
    void (*cleanup)(pollset_t *pollset);

    int (*add)(poll_t *poll);
    int (*remove)(poll_t *poll);

    int (*poll)(pollset_t *pollset, time_t timeout);
    // int (*notify)(pollset_t *pollset);
}pollset_actions_t;
extern pollset_actions_t pollset_actions;

pollset_t *pollset_create(unsigned int capacity);

void pollset_destroy(pollset_t *pollset);


poll_t *pollset_add(pollset_t *pollset, short when,
        int fd, poll_handler_f handler, void *data);

void pollset_remove(poll_t *poll);


#define pollset_poll pollset_actions.poll

#ifdef __cplusplus
}
#endif

#endif