#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>

char package[] = "hello world\n";

int main()
{
	int sock = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
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

	struct sockaddr_in other = {
		.sin_family = AF_INET,
		.sin_port = htons(10001),
		.sin_addr.s_addr = htonl(INADDR_BROADCAST)
	};

	int slen = sizeof(other);

	int numwrite = sendto(sock,package,sizeof(package),0,(struct sockaddr *)&other, slen);
	if (numwrite == -1)
	{
		perror("sendto error");
	}
	else
	{
		printf("send %d bytes\n",numwrite);
	}

	shutdown(sock, 2);
	close(sock);

	return 0;
}

