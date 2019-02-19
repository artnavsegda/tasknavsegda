#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <signal.h>
#include <poll.h>
#include "command.h"
#include "net.h"

#define LIMIT 100

int sock;
int countdown = 0;

struct One buf;
struct Two package = {
	.Temperature = 42,
	.Light = 10000,
};

in_addr_t masteraddr;

void timer_handler(int signal)
{
	struct sockaddr_in other = {
		.sin_family = AF_INET,
		.sin_port = htons(10002),
		.sin_addr.s_addr = htonl(INADDR_BROADCAST)
	};
	int slen = sizeof(other);
	//printf("alarm\n");
	int numwrite = sendto(sock,&package,sizeof(package),0,(struct sockaddr *)&other,slen);

	if (package.Priority && countdown <= LIMIT)
	{
		countdown += package.Priority;
	}
}

void timer_init(void)
{
	struct itimerval it_val;
	it_val.it_value.tv_sec = 5;
	it_val.it_value.tv_usec = 0;
	it_val.it_interval = it_val.it_value;
	setitimer(ITIMER_REAL, &it_val, NULL);
}

void start_minimaster(void)
{
	static char server_started = 0;
	if (!server_started)
	{
		server_started = 1;
	}
}

int main()
{
	sigset_t mask;
	struct timespec timeout = { .tv_sec = 3 };
	sock = socket_init();
	server_init(sock,10001);
	struct pollfd fds[1] = {{ .fd = sock, .events = POLLIN }};
	sigemptyset(&mask);
	sigaddset(&mask,SIGALRM);
	signal(SIGALRM, timer_handler);
	timer_init();

	struct sockaddr_in other;
	int slen = sizeof(other);

	printf("Awaiting master\n");

	while(1)
	{
		if (countdown > LIMIT)
		{
			start_minimaster();
		}



		int ret = ppoll(fds,1,&timeout,&mask);
		if (ret == -1)
		{
			perror("poll");
		}
		else if (ret > 0)
		{
			int numread = recvfrom(sock,&buf,sizeof(buf),0,(struct sockaddr *)&other, &slen);
			if (numread == -1)
			{
				perror("recv error");
			}
			else
			{
				printf("recv %d bytes from address %s port %d\n",numread, inet_ntoa(other.sin_addr), ntohs(other.sin_port));
				printf("Text: %s\nM.Temp: %d\nM.Light: %d\nStatus: %d\nPriority: %d\nNode count: %d\n",buf.Text,buf.Temperature,buf.Light,buf.Status,buf.Priority,buf.Nodecount);
				masteraddr = other.sin_addr.s_addr;
				package.Priority = buf.Priority;
				countdown = 0;
			}
		}
		else if (ret == 0)
		{
			printf("poll timeout\n");
		}
	}

	shutdown(sock, 2);
	close(sock);

	return 0;
}
