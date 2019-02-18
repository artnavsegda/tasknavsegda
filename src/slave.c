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

struct One buf;
struct Two package;

void timer_handler(int signal)
{
	struct sockaddr_in other = {
		.sin_family = AF_INET,
		.sin_port = htons(10002),
		.sin_addr.s_addr = htonl(INADDR_BROADCAST)
	};
	int slen = sizeof(other);
	printf("alarm\n");
	package.Temperature = 42;
	package.Light = 10000;
	int numwrite = sendto(sock,&package,sizeof(package),0,(struct sockaddr *)&other,slen);
}

int main()
{
	struct itimerval it_val;
	it_val.it_value.tv_sec = 5;
	it_val.it_value.tv_usec = 0;
	it_val.it_interval = it_val.it_value;
	signal(SIGALRM, timer_handler);
	setitimer(ITIMER_REAL, &it_val, NULL);

	//unsigned char buf[1000];
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
		.sin_port = htons(10001)
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
			printf("recv %d bytes\n",numread);
			printf("Text: %s\n",buf.Text);
		//	for (int i=0; i<numread;i++)
		//	{
		//		printf("0x%02X, ",buf[i]);
		//	}
		//	printf("\n");
		}
	}

	shutdown(sock, 2);
	close(sock);

	return 0;
}
