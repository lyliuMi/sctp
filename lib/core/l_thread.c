#include "l_thread.h"
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void *thread_worker(void *data)
{
    thread_t *t = (thread_t *)data;
    if(t->running == true)
        return NULL;
    t->running = true;
    t->fun(t->data);
    t->running = false;
    return t;
}

thread_t *thread_create_(func fun, void *data)
{
    thread_t *t = (thread_t *)malloc(sizeof(thread_t));
    if(!t)
    {
        fprintf(stderr, "memory allocate fail\n");
        return NULL;
    }
    memset(t, 0, sizeof(thread_t));

    if(pthread_create(&t->pid, NULL, thread_worker, t) < 0)
    {
        fprintf(stderr, "thread create fail\n");
        return NULL;
    }

    t->running = false;
    t->fun = fun;
    t->data = data;

    return t;
}

int thread_destroy(thread_t *t)
{
    assert(t);
    pthread_join(t->pid, NULL);
    free(t);

    return 0;
}