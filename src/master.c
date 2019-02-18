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
#include "command.h"

int sock;
struct Status primary;
struct Nodestatus * current;

void timer_handler(int signal)
{
	struct One package;
	struct sockaddr_in other = {
		.sin_family = AF_INET,
		.sin_port = htons(10001),
		.sin_addr.s_addr = htonl(INADDR_BROADCAST)
	};
	int slen = sizeof(other);
	package.Temperature = 0;
	package.Light = 0;
	package.Status = 1;
	int numwrite = sendto(sock,&package,sizeof(package),0,(struct sockaddr *)&other,slen);
}

int compare(in_addr_t *x, struct Nodestatus *y)
{
	   return (*x - y->Address);
}

int mediantemp()
{
	int temperature;
	for (int i = 0; i < primary.Nodecount; i++)
		temperature += primary.Nodes[i].Temperature;
	return temperature/primary.Nodecount;
}

int medianlight()
{
	int temperature;
	for (int i = 0; i < primary.Nodecount; i++)
		temperature += primary.Nodes[i].Temperature;
	return temperature/primary.Nodecount;
}

void sendcontrol(struct sockaddr_in other)
{
	int slen = sizeof(other);
	struct One control = {
		.Text = "Digitaaaal",
		.Temperature = mediantemp(),
		.Light = medianlight(),
	};
	int numwrite = sendto(sock,&control,sizeof(control),0,(struct sockaddr *)&other, slen);
}

int main()
{
        struct itimerval it_val;
	it_val.it_value.tv_sec = 5;
	it_val.it_value.tv_usec = 0;
	it_val.it_interval = it_val.it_value;
	signal(SIGALRM, timer_handler);
	//setitimer(ITIMER_REAL, &it_val, NULL);

	struct Two package;
	unsigned char buf[1000];
	sock = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
	if (sock == -1)
	{
		perror("socket error");
		return 1;
	}
	else
	{
		printf("socket ok\n");
	}

	int broadcast = 1;

	setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof broadcast);

	struct sockaddr_in server = {
		.sin_family = AF_INET,
		.sin_addr.s_addr = INADDR_ANY,
		.sin_port = htons(10002)
	};

	struct sockaddr_in other;
	int slen = sizeof(other);

	if (bind(sock,(struct sockaddr *)&server,sizeof(server)) == -1)
	{
		perror("bind error");
		close(sock);
		return 1;
	}
	else
	{
		printf("bind ok\n");
	}

	while(1)
	{
		int numread = recvfrom(sock,&package,sizeof(package),0,(struct sockaddr *)&other, &slen);
		if (numread == -1)
		{
			perror("recv error");
		}
		else
		{
			printf("recv %d bytes from %s\n",numread, inet_ntoa(other.sin_addr));
			printf("temp %d light %d priority %d status %d\n",package.Temperature,package.Light,package.Priority,package.Status);
			current = bsearch(&(other.sin_addr),primary.Nodes,primary.Nodecount,sizeof(struct Nodestatus), (int(*) (const void *, const void *)) compare);
			if (current == NULL)
			{
				printf("Unindentified node\n");
				primary.Nodes[primary.Nodecount].Temperature = package.Temperature;
				primary.Nodes[primary.Nodecount].Light = package.Light;
				primary.Nodes[primary.Nodecount].Address = other.sin_addr.s_addr;
				current = &(primary.Nodes[primary.Nodecount]);
				primary.Nodecount++;
			}
			else
			{
				printf("Node in database\n");
			}
			printf("db status of node temp %d light %d priority %d status %d\n", current->Temperature, current->Light, current->Priority, current->Status);
			sendcontrol(other);
		}
	}

	shutdown(sock, 2);
	close(sock);

	return 0;
}
