#include "l_core.h"

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

extern int server_test_domain_id;
typedef struct app_context_s app_context_t;

static DOUBLY_LIST(poll_list);
static app_context_t context;

typedef struct app_context_s
{
	pollset_t *pollbox;
	int fd;
	int running;
}app_context_t;

#undef LOG_DOMAIN
#define LOG_DOMAIN server_test_domain_id

void signal_handler(int signum) {
    // 处理信号
    printf("Signal %d received\n", signum);
	context.running = false;
}

static void app_init()
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

void handle_recv_from_client_stream(short when, int fd, void *data)
{
	assert(fd != -1);
	char recv_buf[1024] = {0};
	char send_buf[212992] = {0};
	strcpy(send_buf, "asdasdasdasdasdasdasdddddddddddddasdasdasdadasdasdadasdasdadasdasdasd");
	ssize_t count = 0;
	// socklen_t len = 0;
	if(when & _EPOLLIN_)
	{
		printf("[server]: 触发EPOLLIN\n");
		while(1)
		{
			ssize_t nbytes = recv(fd, recv_buf + count, 2, 0);
			if (nbytes == -1) 
			{
				// perror(errno);
				if(errno == EAGAIN)
					break;
				else
				{
      				printf("[server]: send errno = %d: %s\n", errno, strerror(errno));
					exit(1);
				}
			}
			else if (nbytes == 0) 
			{
				// 服务端关闭连接
				printf("[server]: client [%d] disconnected.\n", fd);
				pollset_remove((poll_t *)data);
				doubly_list_remove(&poll_list, (poll_t *)data);
				close(fd);
				break;
				// pollset_remove((poll_t *)data);
			} 
			else
			{
				printf("[server]: [%d] receive: %ld context is [%s]\n", fd, nbytes, recv_buf);
				// printf("[server]: from: %d and dir is [%s]\n", len, client_addr.sun_path);

				count += nbytes;
			} 

		}

	}
	// int i = 0;
	if(when & _EPOLLOUT_)
	{
		printf("[server]: 触发EPOLLOUT\n");
		//printf("[server]: can call send function! %d\n", ++i);
		// printf("[server]: send %d and %s to client \n", count, recv_buf);
		ssize_t sendn = send(fd, send_buf, strlen(send_buf), 0);
		printf("[server]: sendn = %ld\n", sendn);
		if(sendn == -1)
		{
      		printf("[server]: send errno = %d: %s\n", errno, strerror(errno));
		}
		else if(sendn != strlen(send_buf))
			printf("[server]: this send is not enough!\n");
		else
			printf("[server]: this send is enough to %d!\n", fd);
		
	}
}

void handle_recv_from_client_stream2(short when, int fd, void *data)
{
	assert(fd != -1);
	char recv_buf[1024] = {0};
	struct sockaddr_in client_addr;
	memset(&client_addr, 0, sizeof(client_addr));
	socklen_t len = 0;
	struct sctp_sndrcvinfo sndrcvinfo;
	int mflags = 0;
	// socklen_t len = 0;
	if(when & _EPOLLIN_)
	{
		log_info("[server]: 触发EPOLLIN\n");

		ssize_t nbytes = sctp_recvmsg(fd, recv_buf, 1024, (struct sockaddr *)&client_addr, &len, &sndrcvinfo, &mflags);
		if (nbytes == -1) 
		{
			// perror(errno);
			if(errno == EAGAIN)
				return;
			else
			{
				printf("[server]: send errno = %d: %s\n", errno, strerror(errno));
				exit(1);
			}
		}
		else if (nbytes == 0) 
		{
			// 服务端关闭连接
			printf("[server]: client [%d] disconnected.\n", fd);
			pollset_remove((poll_t *)data);
			doubly_list_remove(&poll_list, (poll_t *)data);
			close(fd);
			return;
			// pollset_remove((poll_t *)data);
		} 
		else
			log_info("[server]: [%d] receive: %ld context is [%s]\n", fd, nbytes, recv_buf);

	
		log_info("[server]: connection from:[%d] == %s:%d", fd, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
		if(mflags & MSG_NOTIFICATION)
		{
			union sctp_notification *not = (union sctp_notification *)recv_buf;
			switch(not->sn_header.sn_type) 
			{
        		case SCTP_ASSOC_CHANGE :
            		log_info("SCTP_ASSOC_CHANGE:"
						"[T:%d, F:0x%x, S:%d, I/O:%d/%d]", 
						not->sn_assoc_change.sac_type,
						not->sn_assoc_change.sac_flags,
						not->sn_assoc_change.sac_state,
						not->sn_assoc_change.sac_inbound_streams,
						not->sn_assoc_change.sac_outbound_streams);

					if (not->sn_assoc_change.sac_state == SCTP_COMM_UP) 
					{
						log_info("SCTP_COMM_UP");
					} 
					else if (not->sn_assoc_change.sac_state == SCTP_SHUTDOWN_COMP ||
							not->sn_assoc_change.sac_state == SCTP_COMM_LOST) 
					{
						if (not->sn_assoc_change.sac_state == SCTP_SHUTDOWN_COMP)
							log_info("SCTP_SHUTDOWN_COMP");
						if (not->sn_assoc_change.sac_state == SCTP_COMM_LOST)
							log_info("SCTP_COMM_LOST");

						// s1ap_event_push(MME_EVENT_S1AP_LO_CONNREFUSED,
						// 		sock, addr, NULL, 0, 0);
					}
					break;
				case SCTP_SHUTDOWN_EVENT :
					log_info("SCTP_SHUTDOWN_EVENT:[T:%d, F:0x%x, L:%d]",
							not->sn_shutdown_event.sse_type,
							not->sn_shutdown_event.sse_flags,
							not->sn_shutdown_event.sse_length);

					log_info("MME_EVENT_S1AP_LO_CONNREFUSED");
					break;
				default:
					log_error("Discarding event with unknown flags:0x%x type:0x%x",mflags, not->sn_header.sn_type);					
			}
		}
		else if(mflags & MSG_EOR)
		{
			log_info("MSSAGE %s", recv_buf);
		}
		else
			log_info("others %s", recv_buf);

	}
	printf("\n");

}

void handle_server_accept(short when, int fd, void *data)
{
	assert(fd != -1);
	struct sockaddr_in client_addr;
	socklen_t len;
	pollset_t *pollbox = (pollset_t *)(data);
	// int conn_fd = accept(fd, (struct sockaddr *)&client_addr, &len);
	int conn_fd = accept(fd, NULL, NULL);
	if (conn_fd == -1) 
		perror("accept");
	else 
	{
		log_info("[server]: Accepted new connection:[%d]", conn_fd);
		// log_info("[server]: Accepted new connection:[%d] == %s:%d", conn_fd, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

		// pollset_add(pollbox, _EPOLLIN_ | _EPOLLOUT_, conn_fd, handle_recv_from_client_stream, NULL);
		// poll_t *poll = pollset_add(pollbox, _EPOLLIN_, conn_fd, handle_recv_from_client_stream, NULL);
		poll_t *poll = pollset_add(pollbox, _EPOLLIN_, conn_fd, handle_recv_from_client_stream2, NULL);
		
		assert(poll);
		doubly_list_add(&poll_list, poll);
	
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

static int app_work()
{
    int fd, rv;
    struct sockaddr_in servaddr;
    poll_t *poll = NULL;

	context.running = true;
    fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
	if(fd < 0)
	{
		log_error("create socket fail!");
		close(fd);
		fd = -1;
		return -1;
	}

    servaddr.sin_family = AF_INET;
    if(inet_pton(AF_INET, "192.168.177.129", &servaddr.sin_addr) < 0)
    {
		log_error("inet_pton fail!");
        return -1;
    }
    servaddr.sin_port = htons(36412);


    struct sctp_event_subscribe event_subscribe;
    memset(&event_subscribe, 0, sizeof(event_subscribe));
    event_subscribe.sctp_data_io_event = 1;
    event_subscribe.sctp_association_event = 1;
    event_subscribe.sctp_send_failure_event = 1;
    event_subscribe.sctp_shutdown_event = 1;

    rv = setsockopt(fd, IPPROTO_SCTP, SCTP_EVENTS, &event_subscribe, sizeof(event_subscribe));
    if(rv < 0)
    {
        log_error("errno = %s", strerror(errno));
        return rv;
    }
    rv = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (void *)&fd, sizeof(int));
    if(rv < 0)
    {
        log_error("errno = %s", strerror(errno));
        return rv;
    }

	// rv = setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, (void *)&fd, sizeof(int));
    // if(rv < 0)
    // {
    //     log_error("errno = %s", strerror(errno));
    //     return rv;
    // }

    if(bind(fd, (struct sockaddr *)&servaddr, sizeof(servaddr)) != 0)
    {
        log_error("bind fail!");
        return -1;
    }
    log_info("socket bind %s:%d", inet_ntoa(servaddr.sin_addr), ntohs(servaddr.sin_port));
    ///////////////////////////////

    if(listen(fd, 5) < 0)
    {
        log_error("errno = %s", strerror(errno));
        return rv;
    }
    log_info("socket is listenning");

    ////////////////////
    // socklen_t len;
    // int conn_fd;
	// if((conn_fd = accept(fd, (struct sockaddr *)&client_addr, &len)) < 0)
	// {
	// 	log_error("errno = %s", strerror(errno));
    //     return rv;
	// }
    // log_info("server accept %s:%d", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

    //////////////////////////
	pollset_t* pollbox = context.pollbox;
	assert(pollbox);
    poll = pollset_add(pollbox, _EPOLLIN_, fd, handle_server_accept, pollbox);
    assert(poll);
	doubly_list_add(&poll_list, poll);

	thread_t *t = thread_create_(start_epoll, (void *)pollbox);
	thread_destroy(t);
	////////////////////
	context.fd = fd;
    return 0;
}

static void app_fini()
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
