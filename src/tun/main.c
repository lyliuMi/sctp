#include "l_core.h"
#include "tun.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>

#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/sctp.h>

extern int tun_test_domain_id;
#undef LOG_DOMAIN
#define LOG_DOMAIN tun_test_domain_id

typedef struct app_context_s
{
	pollset_t *pollbox;
	int fd;
	int running;
}app_context_t;

static DOUBLY_LIST(poll_list);
static app_context_t context;
void signal_handler(int signum) {
    // 处理信号
    printf("Signal %d received\n", signum);
	context.running = false;
}

void app_init()
{
    DOUBLY_LIST_INIT(&poll_list);
    memset(&context, 0, sizeof(context));
	context.pollbox = pollset_create(16);
	context.fd = -1;
	context.running = false;
	// signal(SIGINT, signal_handler); // 捕捉 Ctrl+C 信号	
	struct sigaction sa;
	sa.sa_handler = signal_handler;
	sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0; // 禁用 SA_RESTART
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }
}

void start_epoll(void *data)
{	
	int rv;
	pollset_t *pollbox = (pollset_t *)data;
	while(true)
	{		
		if(context.running == false)
			break;
		rv = pollset_poll(pollbox, 1000);
		if(rv < 0)
		{
			if(rv == -1)
			{		
				perror("epoll_wait return -1!");
				break;
			}
			else if(rv == -2)
				continue;
		}


	}
}

void recv_from_vni(short when, int fd, void *data)
{
    assert(fd != -1);
	char recv_buf[1024] = {0};
    ssize_t count = 0;
	// socklen_t len = 0;
	if(when & _EPOLLIN_)
	{
		printf("[server]: 触发EPOLLIN\n");
		while(1)
		{
			ssize_t nbytes = read(fd, recv_buf + count, 1024);
			if (nbytes == -1) 
			{
				// perror(errno);
				if(errno == EAGAIN)
					break;
				else
				{
      				printf("[tun]: send errno = %d: %s\n", errno, strerror(errno));
					exit(1);
				}
			}
			else if (nbytes == 0) 
			{
				// 服务端关闭连接
				printf("[tun]: client [%d] disconnected.\n", fd);
				pollset_remove((poll_t *)data);
				doubly_list_remove(&poll_list, (poll_t *)data);
				close(fd);
				break;
				// pollset_remove((poll_t *)data);
			} 
			else
			{
				// printf("[tun]: [%d] receive: %ld context is [%s]\n", fd, nbytes, recv_buf);
				// printf("[server]: from: %d and dir is [%s]\n", len, client_addr.sun_path);

				count += nbytes;
			} 

		}
        printf("[tun]: [%d] receive: %ld bytes\n", fd, count);

        for(int i = 0; i < count; i++)
        {
            printf("%x ", recv_buf[i]);
        }

	}
}

void app_work()
{
    int fd = tun_open("vni_nr", 0);
    set_tun_ip("vni_nr", "192.168.177.100", "255.255.255.255");

	context.running = true;
	pollset_t* pollbox = context.pollbox;
	assert(pollbox);
    poll_t *poll = pollset_add(pollbox, _EPOLLIN_, fd, recv_from_vni, pollbox);
    assert(poll);
	doubly_list_add(&poll_list, poll);

	thread_t *t = thread_create_(start_epoll, (void *)pollbox);
	thread_destroy(t);
	////////////////////
	context.fd = fd;
}


void app_fini()
{
    log_info("start destroy resources");
	poll_t *node;

	doubly_list_for_each(&poll_list, node)
		pollset_remove(node);

	close(context.fd);
	pollset_destroy(context.pollbox);
}

int work(int argc, char *argv[])
{
    app_init();

    app_work();

    app_fini();
    return 0;
}
