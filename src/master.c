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

int sock;
struct Status primary;

int priorityroll = 1;

int compare(in_addr_t *x, struct Nodestatus *y)
{
	   return (*x - y->Address.s_addr);
}

int mediantemp()
{
	int temperature = 0;
	for (int i = 0; i < primary.Nodecount; i++)
		temperature += primary.Nodes[i].Temperature;
	return temperature/primary.Nodecount;
}

int medianlight()
{
	int light = 0;
	for (int i = 0; i < primary.Nodecount; i++)
		light += primary.Nodes[i].Light;
	return light/primary.Nodecount;
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
	control.Nodecount = primary.Nodecount;
	memcpy(control.Nodes,primary.Nodes,sizeof(struct Nodestatus)*MAXNODE);
	int numwrite = sendto(sock,&control,sizeof(control),0,(struct sockaddr *)&other, slen);
}

void timer_handler(int signal)
{
	struct Two package;
	struct sockaddr_in other = {
		.sin_family = AF_INET,
		.sin_port = htons(10002),
		.sin_addr.s_addr = htonl(INADDR_BROADCAST)
	};
	int slen = sizeof(other);
	printf("alarm\n");
	package.Temperature = 0;
	package.Light = 0;
	package.Status = 1;
	package.Priority = 0;
	int numwrite = sendto(sock,&package,sizeof(package),0,(struct sockaddr *)&other,slen);
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
	struct Two package;
	struct timespec timeout = { .tv_sec = 3 };
	sigset_t mask;
	unsigned char buf[1000];

	sock = socket_init();
	server_init(sock,10002);
	struct pollfd fds[1] = {{ .fd = sock, .events = POLLIN }};
	sigemptyset(&mask);
	sigaddset(&mask,SIGALRM);
	signal(SIGALRM, timer_handler);
	timer_init();

	struct sockaddr_in other;
	int slen = sizeof(other);

	while(1)
	{
		int ret = ppoll(fds,1,&timeout,&mask);
		if (ret == -1)
		{
			perror("poll");
		}
		else if (ret > 0)
		{
			int numread = recvfrom(sock,&package,sizeof(package),MSG_DONTWAIT,(struct sockaddr *)&other, &slen);
			if (numread == -1)
			{
				perror("recv error");
			}
			else
			{
				printf("recv %d bytes from address %s port %d\n",numread, inet_ntoa(other.sin_addr), ntohs(other.sin_port));
				printf("temp %d light %d priority %d status %d\n",package.Temperature,package.Light,package.Priority,package.Status);

				if (package.Status != 1)
				{
					current = bsearch(&(other.sin_addr),primary.Nodes,primary.Nodecount,sizeof(struct Nodestatus), (int(*) (const void *, const void *)) compare);
					if (current == NULL)
					{
						printf("Unindentified node\n");
						primary.Nodes[primary.Nodecount].Address.s_addr = other.sin_addr.s_addr;
						primary.Nodes[primary.Nodecount].Priority = priorityroll++;
						current = &(primary.Nodes[primary.Nodecount]);
						primary.Nodecount++;
					}
					else
					{
						printf("Node in database\n");
					}
					current->Temperature = package.Temperature;
					current->Light = package.Light;
					current->Status = package.Status;
					printf("db status of node temp %d light %d priority %d status %d ip %s\n", current->Temperature, current->Light, current->Priority, current->Status, inet_ntoa(current->Address));
					sendcontrol(other.sin_addr.s_addr,current);
				}
				else
				{
					//printf("Reflection refracted\n");
				}
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
