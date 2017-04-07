#include <sys/socket.h>
#include <stdio.h>
#include <string.h>
#include <netinet/ip.h>
#include <unistd.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
	int sock, nbytes, flags, addrlen, size, i, n, iPort;
	struct sockaddr_in server, from;
	char port[6], buffer[500];
	u_char ttl;

	if(argc != 2)
	{
		printf("%d", argc);
		printf("Expected one command, group number, to be passed to program\n");
		return -1;
	}

	//Take the port in from the command line
	memset(port, 222, sizeof(char) * 3); 
	strcat(port, argv[1]);
	//Creat socket
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if(sock < 0)
	{
		printf("Socket result: %d", sock);
		return -1;
	}
	memset(buffer, '\0', sizeof(buffer));
	ttl = 250;
	iPort = atoi(port);
	server.sin_port = htons(iPort);
	//Bind the socket to the server
	i = bind(sock, (struct sockaddr *) &server, sizeof(server));
	if(i < 0)
	{
		printf("Bind result: %d\n", i);
		return -1;
	}
	//Set the socket to be a multicast socket
	setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl));
	//Set server structure information
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = htonl(INADDR_ANY);
	//If the bind didn't fail then the server is ready to recieve a message
	printf("Multicast UDP is ready\n\n");
	//Set some variables for the recvfrom call
	addrlen = sizeof(from);
	nbytes = 500;
	flags = 0;
	n = 0;
	while(1)
	{
		//Recieve a message, setting the bytes recieve to size, and 
		//storing the message at buffer
		size = send(sock,(char*) n, sizeof(int), flags);
		printf("Message sent: \"%d\"\n\n", (int) ((char *) n));
		n++;
		sleep(1);
	}
}
