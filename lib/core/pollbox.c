#include "pollbox.h"
#include <stdbool.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

extern const pollset_actions_t epoll_actions;
extern const pollset_actions_t select_actions;

pollset_actions_t pollset_actions;
bool pollset_actions_initialized = false;

pollset_t *pollset_create(unsigned int capacity)
{
    pollset_t *pollset = calloc(1, sizeof(pollset_t));
    if (!pollset) 
    {
        fprintf(stderr, "pollset_create() failed");
        return NULL;
    }

    pollset->capacity = capacity;

    memcpy(&pollset_actions, &epoll_actions, sizeof(pollset_actions_t));

    mempool_init(&pollset->pool, capacity);

    if (pollset_actions_initialized == false) 
        pollset_actions_initialized = true;

    pollset_actions.init(pollset);

    return pollset;
}

void pollset_destroy(pollset_t *pollset)
{
    assert(pollset);

    pollset_actions.cleanup(pollset);

    mempool_destroy(&pollset->pool);
    free(pollset);
}

poll_t *pollset_add(pollset_t *pollset, short when,
        int fd, poll_handler_f handler, void *data)
{
    poll_t *poll = NULL;
    int rc;

    assert(pollset);

    assert(fd != -1);
    assert(handler);

    mempool_alloc(&pollset->pool, &poll);
    assert(poll);

	int flags = fcntl(fd, F_GETFL, 0);
    if(flags == -1)
    {
        perror("fcntl F_GETFL fail!");
        return NULL;
    }
    if(fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
    {
        perror("fcntl error!");
        return NULL;
    }
    // ogs_assert(rc == OGS_OK);
    // rc = ogs_closeonexec(fd);
    // ogs_assert(rc == OGS_OK);

    poll->when = when;
    poll->fd = fd;
    poll->handler = handler;
    if(!data)
        poll->data = poll;
    else
        poll->data = data;

    poll->pollset = pollset;

    rc = pollset_actions.add(poll);
    if (rc != 0) 
    {
        fprintf(stderr, "cannot add poll");
        mempool_free(&pollset->pool, poll);
        return NULL;
    }
    return poll;
}

void pollset_remove(poll_t *poll)
{
    int rc;
    pollset_t *pollset = NULL;

    assert(poll);
    pollset = poll->pollset;
    assert(pollset);

    rc = pollset_actions.remove(poll);
    if (rc != 0) 
        fprintf(stderr, " cannot delete poll %d\n", poll->fd);
    
    mempool_free(&pollset->pool, poll);
}

