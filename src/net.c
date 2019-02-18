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

int socket_init(void)
{
	int setsock;
	setsock = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
	if (setsock == -1)
	{
		perror("socket error");
		exit(1);
	}
	else
	{
		printf("socket ok\n");
	}
	int broadcast = 1;
	setsockopt(setsock, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof broadcast);
	return setsock;
}

void server_init(int setsock, int port)
{
	struct sockaddr_in server = {
		.sin_family = AF_INET,
		.sin_addr.s_addr = INADDR_ANY,
		.sin_port = htons(port)
	};

	if (bind(setsock,(struct sockaddr *)&server,sizeof(server)) == -1)
	{
		perror("bind error");
		close(setsock);
		exit(1);
	}
	else
	{
		printf("bind ok\n");
	}
}
