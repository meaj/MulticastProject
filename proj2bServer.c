#include <arpa/inet.h>
#include <netinet/ip.h>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// maximum number of group members
#define MAX_CLIENTS 10

struct clientServerStorage{
	int nodeNumber;
	int upToDate; //default to 0, when initial data is sent from server. set to 1 when TCP verifies
	int messageVerification; 
};

struct toServerPackage{
	int ipaddress;
	int messageVerification;
	int nodeNumber;
};

struct toClientPackage{
	int groupNumber;
	int messageNumber;
	int port;
	char ipaddress[20];
	int attempt;
};

struct clientState{
	int nodeNumber;
	int messageNumber;
};

//Only allocating up to MAX_CLIENTS client states
struct clientServerStorage clientMessageState[MAX_CLIENTS];
int numOfClients = 0;

int tRet;
pthread_t threads[15];
int threadCount = 0;

int globalSocket;
int globalPort;
int globalCounter;
int globalNodeIndex = 0;
int attemptNumber = 1;

int validateStates(){
	int i;
	if( numOfClients == 0){ //means no states to validate
			return 1; 
	}
	for( i =0; i < numOfClients; i++){
		if( clientMessageState[i].messageVerification != globalCounter){
				return -1;
		}
	}
	return 1;
	//return 1 if all states are up to date with globalCounter
}

void *handle_clients(void *socke){
	int sock = (intptr_t)socke;
	int index = globalNodeIndex;
	globalNodeIndex++;
	//commence as usual.
	while(1){
		struct toServerPackage recvMessage;
		int y;
		y = read(sock,(struct toServerPackage *)&recvMessage, sizeof(recvMessage));
		if(y <0){
				printf("recv error from node %i\n",recvMessage.nodeNumber);
		}
		if(y == 0){
				printf("node %i has disconnected\n",clientMessageState[index].nodeNumber);
				break;
		}
		clientMessageState[index].nodeNumber = recvMessage.nodeNumber;
		clientMessageState[index].messageVerification = recvMessage.messageVerification;
		usleep(600000);
	}
}

/**************************************************/
//RCV INFO//
//info received from clients
//spin off new client 
/**************************************************/
void *rcvInfo(void *threadID){ 
	//SHOULD ACTUALLY BE ABLE TOHANDLE ALL TCP INVOMING
	//CLIENTS ESTABLISH TCP CONN WITH INFO GIVEN BY SERVER UDP PACKETS.
	int sock, i, addrlen, i_socket;
	struct sockaddr_in server;
	server.sin_family = PF_INET;
	server.sin_addr.s_addr = htonl(INADDR_ANY); 
	server.sin_port= htons(globalPort);
	struct sockaddr_storage addr;

	addrlen = sizeof(addr);
	sock = socket(AF_INET,SOCK_STREAM,0);
	if(sock < 0) {
			printf("socket error = %d\n", sock);
	}
	if(numOfClients == 0){
		i = bind(sock, (struct sockaddr*) &server, sizeof(server));
		if( i< 0) {
			printf("bind result: %d\n", i);
		}
	}

	//control statements
	if(listen(sock, MAX_CLIENTS)<0){
		printf("Listen Error\n");
	}
	while((i_socket = accept(sock,(struct sockaddr*)&addr,&addrlen))){
		int new_i;

		tRet = pthread_create(&threads[threadCount], NULL, handle_clients, (void *)(intptr_t)i_socket);
		threadCount++;
		numOfClients++;
	}
}

int main(int argc, char *argv[]){
	int sock, nbytes, flags, addrlen, size, i, n, iPort;
	struct sockaddr_in server, from;
	char group[12], port[6], buffer[500];
	u_char ttl;
	globalPort = 54000;
	if(argc != 2){
		printf("%d ", argc);
		printf("Expected one command, group number, to be passed to program\n");
		return -1;
	}
	strcpy(group, "239.10.5.");
	strcat(group, argv[1]);
	//Take the port in from the command line
	memset(port, '\0', sizeof(port));
	memset(port, '2', sizeof(char) * 3);
	strcat(port, argv[1]);
	//Create socket
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if(sock < 0){
		printf("Socket result: %d", sock);
		return -1;
	}
	memset(buffer, '\0', sizeof(buffer));
	ttl = 250;
	iPort = atoi(port);
	server.sin_port = htons(iPort);

	//Set the socket to be a multicast socket
	setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl));

	//Set server structure information
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = inet_addr(group);

	//If the bind didn't fail then the server is ready to recieve a message
	printf("Multicast UDP is ready\n\n");

	//Set some variables for the recvfrom call
	addrlen = sizeof(from);
	nbytes = 500;
	flags = 0;
	globalCounter = 0;

	//SETUP FOR INITIAL MULTITHREAD RECVFROM//
	tRet = pthread_create(&threads[threadCount], NULL, rcvInfo, (void *)0);
	threadCount++;

	globalSocket = sock;
	if(globalSocket < 0 ){
		printf("globalSocket error = %d\n",globalSocket);
		return -1;
	}
	//SENDS OUT DATA TO MULTICAST GROUP//
	struct toClientPackage message;
	while(1){
		message.groupNumber = 10;
		message.messageNumber = globalCounter;
		message.port = globalPort;
		message.attempt = attemptNumber;
		usleep(100000);//To ensure some variables update
		char ip[20];
		strcpy(message.ipaddress, "10.1.2.3");

		size = sendto(sock, &message, sizeof(message), flags, (struct sockaddr *) &server, sizeof(server));
		if(size < 0){
				perror("Sendto");
				exit(1);
		}
		printf("Broadcasting message number %i\n", message.messageNumber);
		usleep(500000); //half second wait
		int m = validateStates();
		if( m  == 1){
				globalCounter++;
				attemptNumber = 1;
		}
		else{
				attemptNumber++;
		}
		sleep(1);
	}
	close(sock);
}
