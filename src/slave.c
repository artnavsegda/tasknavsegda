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
#include "net.h"

int sock;

struct One buf;
struct Two package;

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
	package.Temperature = 42;
	package.Light = 10000;
	package.Status = 0;
	package.Priority = 0;
	int numwrite = sendto(sock,&package,sizeof(package),0,(struct sockaddr *)&other,slen);
}

void timer_init(void)
{
	struct itimerval it_val;
	it_val.it_value.tv_sec = 5;
	it_val.it_value.tv_usec = 0;
	it_val.it_interval = it_val.it_value;
	signal(SIGALRM, timer_handler);
	setitimer(ITIMER_REAL, &it_val, NULL);
}

int main()
{
	timer_init();
	sock = socket_init();
	server_init(sock,10001);

	struct sockaddr_in other;
	int slen = sizeof(other);

	printf("Awaiting master\n");

	while(1)
	{
		int numread = recvfrom(sock,&buf,sizeof(buf),0,(struct sockaddr *)&other, &slen);
		if (numread == -1)
		{
			perror("recv error");
		}
		else
		{
			printf("recv %d bytes from address %s port %d\n",numread, inet_ntoa(other.sin_addr), ntohs(other.sin_port));
			printf("Text: %s\n",buf.Text);
			printf("M.Temp: %d\n",buf.Temperature);
			printf("M.Light: %d\n",buf.Light);
			printf("Status: %d\n",buf.Status);
			printf("Priority: %d\n",buf.Priority);
			printf("Node count: %d\n",buf.Nodecount);
			masteraddr = other.sin_addr.s_addr;
		}
	}

	shutdown(sock, 2);
	close(sock);

	return 0;
}
