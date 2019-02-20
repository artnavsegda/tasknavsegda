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

#define LIMIT 3

int sock, sock2;
int countdown = 0;

struct One buf;
struct Two package = {
	.Temperature = 42,
	.Light = 10000,
	.Status = 0,
	.Priority = 0
};
struct Two package2;

in_addr_t masteraddr;

int compare(in_addr_t *x, struct Nodestatus *y)
{
	   return (*x - y->Address.s_addr);
}

int mediantemp()
{
	int temperature = 0;
	for (int i = 0; i < buf.Nodecount; i++)
		temperature += buf.Nodes[i].Temperature;
	return temperature/buf.Nodecount;
}

int medianlight()
{
	int light = 0;
	for (int i = 0; i < buf.Nodecount; i++)
		light += buf.Nodes[i].Light;
	return light/buf.Nodecount;
}

void sendcontrol(in_addr_t Address, struct Nodestatus * current)
{
	struct sockaddr_in other = {
		.sin_family = AF_INET,
		.sin_port = htons(10001),
		.sin_addr.s_addr = Address
	};

	int slen = sizeof(other);
	struct One control = {
		.Text = "Digitaaaal"
	};
	control.Temperature = mediantemp();
	control.Light = medianlight();
	control.Time = 0;
	control.Status = 0;
	control.Address = Address;
	control.Priority = current->Priority;
	control.Nodecount = buf.Nodecount;
	memcpy(control.Nodes,buf.Nodes,sizeof(struct Nodestatus)*MAXNODE);
	int numwrite = sendto(sock,&control,sizeof(control),0,(struct sockaddr *)&other, slen);
}

void timer_handler(int signal)
{
	struct sockaddr_in other = {
		.sin_family = AF_INET,
		.sin_port = htons(10002),
		.sin_addr.s_addr = htonl(INADDR_BROADCAST)
	};
	int slen = sizeof(other);
	printf("Internal status is: %d\n", package.Status);
	int numwrite = sendto(sock,&package,sizeof(package),0,(struct sockaddr *)&other,slen);

	if (package.Priority && countdown <= LIMIT)
	{
		countdown += package.Priority;
		printf("%d steps to minimaster\n",LIMIT-countdown);
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

int main()
{
	struct Nodestatus * current;
	sigset_t mask;
	struct timespec timeout = { .tv_sec = 3 };
	sock = socket_init();
	sock2 = socket_init();
	server_init(sock,10001);
	server_init(sock2,10002);
	struct pollfd fds[1] = {{ .fd = sock, .events = POLLIN }};
	struct pollfd fds2[1] = {{ .fd = sock2, .events = POLLIN }};
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
			package.Status = 2;
			int ret = ppoll(fds2,1,&timeout,&mask);
			if (ret == -1)
			{
				perror("poll");
			}
			else if (ret > 0)
			{
				int numread = recvfrom(sock2,&package2,sizeof(package2),MSG_DONTWAIT,(struct sockaddr *)&other, &slen);
				if (numread == -1)
				{
					perror("recv error");
				}
				else
				{
					if (package2.Priority == buf.Priority && package2.Status == 2)
					{
						//printf("Reflection refracted\n");
					}
					else if (package2.Priority > buf.Priority || package2.Status == 1)
					{
						countdown == 0;
					}
					else
					{
						current = bsearch(&(other.sin_addr),buf.Nodes,buf.Nodecount,sizeof(struct Nodestatus), (int(*) (const void *, const void *)) compare);
						if (current)
						{
							sendcontrol(other.sin_addr.s_addr,current);
						}
					}
				}
			}
			else if (ret == 0)
			{
				printf("minimaster poll timeout\n");
			}
		}
		else
		{
			package.Status = 0;
			int ret = ppoll(fds,1,&timeout,&mask);
			if (ret == -1)
			{
				perror("poll");
			}
			else if (ret > 0)
			{
				int numread = recvfrom(sock,&buf,sizeof(buf),MSG_DONTWAIT,(struct sockaddr *)&other, &slen);
				if (numread == -1)
				{
					perror("recv error");
				}
				else
				{
					printf("recv %d bytes from address %s port %d\n",numread, inet_ntoa(other.sin_addr), ntohs(other.sin_port));
					printf("Text: %s\nM.Temp: %d\nM.Light: %d\nStatus: %d\nPriority: %d\nNode count: %d\n Node list:\n",buf.Text,buf.Temperature,buf.Light,buf.Status,buf.Priority,buf.Nodecount);
					for (int i = 0; i < buf.Nodecount; i++)
					{
						printf("%d. IP: %s, PR: %d, ST: %d, T: %d, L: %d\n", i, inet_ntoa(buf.Nodes[i].Address), buf.Nodes[i].Priority, buf.Nodes[i].Status, buf.Nodes[i].Temperature, buf.Nodes[i].Light);
					}
					masteraddr = other.sin_addr.s_addr;
					package.Priority = buf.Priority;
					countdown = 0;
				}
			}
			else if (ret == 0)
			{
				printf("slave poll timeout\n");
			}
		}
	}

	shutdown(sock, 2);
	close(sock);

	return 0;
}
