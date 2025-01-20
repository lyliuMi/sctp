#ifndef _L_THREAD_H
#define _L_THREAD_H

#include <pthread.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif


typedef void (*func) (void *data);

typedef struct thread_s
{
    pthread_t pid;
    bool running;
    func fun;
    void *data;
}thread_t;

thread_t *thread_create_(func fun, void *data);


int thread_destroy(thread_t *t);


#ifdef __cplusplus
}
#endif

#endif