#include <unistd.h>
#include <sys/epoll.h>
#include "pollbox.h"
#include "l_hash.h"
#include <errno.h>
#include <string.h>

static void epoll_init(pollset_t *pollset);
static void epoll_cleanup(pollset_t *pollset);
static int epoll_add(poll_t *poll);
static int epoll_remove(poll_t *poll);
static int epoll_process(pollset_t *pollset, time_t timeout);


const pollset_actions_t epoll_actions = {
    epoll_init,
    epoll_cleanup,

    epoll_add,
    epoll_remove,
    epoll_process
};

struct epoll_map_s 
{
    poll_t *read;
    poll_t *write;
};

struct epoll_context_s
{
    int epfd;
    hash_t *map_hash;
    struct epoll_event *event_list;
};

static void epoll_init(pollset_t *pollset)
{
    struct epoll_context_s *context = NULL;
    assert(pollset);

    context = calloc(1, sizeof *context);
    assert(context);
    pollset->context = context;

    context->event_list = calloc(pollset->capacity, sizeof(struct epoll_event));
    assert(context->event_list);

    context->map_hash = hash_make();
    assert(context->map_hash);

    context->epfd = epoll_create(pollset->capacity);
    if (context->epfd < 0) 
    {
        fprintf(stderr, "epoll_create() failed [%d]", pollset->capacity);
        abort();
        return;
    }

    // ogs_notify_init(pollset);
}

static void epoll_cleanup(pollset_t *pollset)
{
    struct epoll_context_s *context = NULL;

    assert(pollset);
    context = pollset->context;
    assert(context);

    // ogs_notify_final(pollset);
    close(context->epfd);
    free(context->event_list);
    hash_destroy(context->map_hash);

    free(context);
}

static int epoll_add(poll_t *poll)
{
    int rv, op;
    pollset_t *pollset = NULL;
    struct epoll_context_s *context = NULL;
    struct epoll_map_s *map = NULL;
    struct epoll_event ee;

    assert(poll);
    pollset = poll->pollset;
    assert(pollset);
    context = pollset->context;
    assert(context);

    map = hash_get(context->map_hash, &poll->fd, sizeof(poll->fd));
    if (!map) 
    {
        map = calloc(1, sizeof(*map));
        if (!map) 
        {
            perror("calloc() failed");
            return -1;
        }

        op = EPOLL_CTL_ADD;
        hash_set(context->map_hash, &poll->fd, sizeof(poll->fd), map);
    } else {
        op = EPOLL_CTL_MOD;
    }

    if (poll->when & _EPOLLIN_)
        map->read = poll;
    if (poll->when & _EPOLLOUT_)
        map->write = poll;

    memset(&ee, 0, sizeof ee);

    ee.events = 0;
    // ee.events |= EPOLLET;
    if (map->read)
        ee.events |= (EPOLLIN|EPOLLRDHUP);
    if (map->write)
        ee.events |= EPOLLOUT;
    ee.data.fd = poll->fd;
    
    //
    printf("epoll_add %p \n", poll);
    int size = 4;
    printf("epoll_add_hash %d \n", _hashfunc_default((void *)&poll->fd, (void *)&size) % 16);

    //
    rv = epoll_ctl(context->epfd, op, poll->fd, &ee);
    if (rv < 0) 
    {
        fprintf(stderr, "epoll_ctl[%d] failed", op);
        return -1;
    }

    return 0;
}

static int epoll_remove(poll_t *poll)
{
    int rv, op;
    pollset_t *pollset = NULL;
    struct epoll_context_s *context = NULL;
    struct epoll_map_s *map = NULL;
    struct epoll_event ee;

    assert(poll);
    pollset = poll->pollset;
    assert(pollset);
    context = pollset->context;
    assert(context);

    map = hash_get(context->map_hash, &poll->fd, sizeof(poll->fd));
    assert(map);

    if (poll->when & _EPOLLIN_)
        map->read = NULL;
    if (poll->when & _EPOLLOUT_)
        map->write = NULL;

    memset(&ee, 0, sizeof ee);

    ee.events = 0;
    if (map->read)
        ee.events |= (EPOLLIN|EPOLLRDHUP);
    if (map->write)
        ee.events |= EPOLLOUT;

    if (map->read || map->write) {
        op = EPOLL_CTL_MOD;
        ee.data.fd = poll->fd;
    } else {
        op = EPOLL_CTL_DEL;
        ee.data.fd = -1;

        hash_set(context->map_hash, &poll->fd, sizeof(poll->fd), NULL);
        free(map);
    }

    rv = epoll_ctl(context->epfd, op, poll->fd, &ee);
    if (rv < 0) 
    {
        fprintf(stderr, "epoll_remove[%d] failed", op);
        return -1;
    }

    return 0;
}

static int epoll_process(pollset_t *pollset, time_t timeout)
{
    struct epoll_context_s *context = NULL;
    int num_of_poll;
    int i;

    assert(pollset);
    context = pollset->context;
    assert(context);

    num_of_poll = epoll_wait(context->epfd, context->event_list, pollset->capacity,
            timeout == -1 ? -1 : ((timeout) ? (timeout / 1000) : 0));
    if (num_of_poll < 0) 
    {
        fprintf(stderr, "errno = %s [epoll failed]", strerror(errno));
        return -1;
    } else if(num_of_poll == 0) 
        return -2;
    
    for (i = 0; i < num_of_poll; i++) 
    {
        struct epoll_map_s *map = NULL;
        uint32_t received;
        short when = 0;
        int fd;

        received = context->event_list[i].events;
        if (received & EPOLLERR) 
            when = _EPOLLIN_;
        else if ((received & EPOLLHUP) && !(received & EPOLLRDHUP)) 
            when = _EPOLLIN_|_EPOLLOUT_;
        else 
        {
            if (received & EPOLLIN) 
                when |= _EPOLLIN_;
            if (received & EPOLLOUT) 
                when |= _EPOLLOUT_;
            if (received & EPOLLRDHUP) 
            {
                when |= _EPOLLIN_;
                when &= ~_EPOLLOUT_;
            }
        }
        if (!when)
            continue;

        fd = context->event_list[i].data.fd;
        assert(fd != -1);
        //
        // printf("epoll_wait %p \n", &fd);
        // printf("epoll_wait_hash %d \n", Times33Hash(&fd, 4) % 16);

        // printf("epoll_wait %p \n", context->event_list[i].data.ptr);

        //
        map = hash_get(context->map_hash, &fd, sizeof(fd));
        if (!map) continue;

        //
        // printf("map->read = %d\n", map->read->fd);
        //
        if (map->read && map->write && map->read == map->write) 
            map->read->handler(when, map->read->fd, map->read->data);
        else 
        {
            if ((when & _EPOLLIN_) && map->read)
                map->read->handler(when, map->read->fd, map->read->data);
            /*
             * map->read->handler() can call ogs_remove_epoll()
             * So, we need to check map instance
             */
            map = hash_get(context->map_hash, &fd, sizeof(fd));
            if (!map) continue;

            if ((when & _EPOLLOUT_) && map->write)
                map->write->handler(when, map->write->fd, map->write->data);
        }
    }
    
    return 0;
}
