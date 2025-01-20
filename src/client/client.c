#include "l_core.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/sctp.h>

extern int client_test_domain_id;
typedef struct app_context_s app_context_t;

static DOUBLY_LIST(poll_list);
static app_context_t context;

typedef struct app_context_s
{
	pollset_t *pollbox;
	int fd;
	int state;
	sctp_assoc_t sac_assoc_id;
}app_context_t;


#undef LOG_DOMAIN
#define LOG_DOMAIN client_test_domain_id
static void app_init()
{
	DOUBLY_LIST_INIT(&poll_list);
	memset(&context, 0, sizeof(context));
	context.pollbox = pollset_create(16);
	context.fd = -1;
	context.state = false;

}

void handle_recv_from_stdin_stream(short when, int fd, void *data)
{
	assert(fd != -1);
	char recv_buf[1024] = {0};
	int client_fd = *((int*)data);
	struct sctp_sndrcvinfo info;
	memset(&info, 0, sizeof(info));
	info.sinfo_assoc_id = context.sac_assoc_id;

	if(when & _EPOLLIN_)
	{
		if(fgets(recv_buf, 1024, stdin) != NULL)
		{	
			printf("[client]: receive from stdin: %s\n", recv_buf);
			sctp_send(client_fd, recv_buf, strlen(recv_buf)-1, &info, 0);
		};

	}
}

void handle_recv_from_server_stream(short when, int fd, void *data)
{
	assert(fd != -1);
	char recv_buf[212992] = {0};
	ssize_t count = 0;
	if(when & _EPOLLIN_)
	{
		while(1)
		{
			ssize_t nbytes = recv(fd, recv_buf + count, 2, 0);
			if(nbytes == -1) 
			{
				if(errno == EAGAIN || errno == EWOULDBLOCK)
				{
      				printf("[client]: recvfrom errno = %d: %s\n", errno, strerror(errno));
					break;
				}
				else
					exit(1);
			}
			else if (nbytes == 0) 
			{
				// 服务端关闭连接
				printf("Server disconnected.\n");
				pollset_remove((poll_t *)data);
				doubly_list_remove(&poll_list, (poll_t *)data);
				close(fd);
				context.state = false;
				break;
			} 
			else 
			{
				printf("[client]: receive: %ld and context is [%s]\n", nbytes, recv_buf);
				count += nbytes;
			}
			// sleep(1);
		}
		printf("[client]: finally receive: %ld and [%s]\n", count, recv_buf);

	}
}

void handle_recv_from_server_stream2(short when, int fd, void *data)
{
	assert(fd != -1);
	char recv_buf[1024] = {0};
	struct sctp_sndrcvinfo sndrcvinfo;
	int mflags = 0;
	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	socklen_t len = 0;
	// socklen_t len = 0;
	if(when & _EPOLLIN_)
	{
		printf("[server]: 触发EPOLLIN\n");

		ssize_t nbytes = sctp_recvmsg(fd, recv_buf, 1024, (struct sockaddr *)&server_addr, &len, &sndrcvinfo, &mflags);
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
			printf("[server]: [%d] receive: %ld context is [%s]\n", fd, nbytes, recv_buf);
			// printf("[server]: from: %d and dir is [%s]\n", len, client_addr.sun_path);
		
		log_info("[client]: from:[%d] == %s:%d", fd, inet_ntoa(server_addr.sin_addr), ntohs(server_addr.sin_port));
		
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
						context.sac_assoc_id = not->sn_assoc_change.sac_assoc_id;

					} else if (not->sn_assoc_change.sac_state == SCTP_SHUTDOWN_COMP ||
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

void start_epoll(void *data)
{	
	int rv;
	pollset_t *pollbox = (pollset_t *)data;
	while(true)
	{
		if(context.state == false)
		{
			log_info("server is done");
			break;
		}
		rv = pollset_poll(pollbox, 100000);
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

	context.state = true;
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

    log_info("socket connect %s:%d", inet_ntoa(servaddr.sin_addr), ntohs(servaddr.sin_port));
    ///////////////////////////////
	if((rv = connect(fd, (struct sockaddr *)&servaddr, sizeof(struct sockaddr_in))) < 0)
	{
		log_error("errno = %s", strerror(errno));
        return rv;
	}
    //////////////////////////
	pollset_t* pollbox = context.pollbox;
	assert(pollbox);
	poll_t *poll = pollset_add(pollbox, _EPOLLIN_, STDIN_FILENO, handle_recv_from_stdin_stream, &fd);
    poll_t *poll1 = pollset_add(pollbox, _EPOLLIN_, fd, handle_recv_from_server_stream2, NULL);
	assert(poll);
    assert(poll1);
	doubly_list_add(&poll_list, poll);
	doubly_list_add(&poll_list, poll1);

	thread_t *t = thread_create_(start_epoll, (void *)pollbox);
	thread_destroy(t);
	//////////////////
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
