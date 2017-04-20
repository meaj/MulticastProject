#include <sys/socket.h>
#include <stdio.h>
#include <string.h>
#include <netinet/ip.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/fcntl.h>
#include <pthread.h>

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

//Only allocating up to 10 client state's
struct clientServerStorage clientMessageState[10];
int numOfClients = 0;

int tRet;
pthread_t threads[15];
int threadCount = 0;

int globalSocket;
int globalPort;
int globalCounter;
int globalNodeIndex = 0;
int attemptNumber = 1;
//gcc -g -pthread project2bmulticastServer.c -o testrun
//May need global array to monitor inactive clients?//
//************* THOUGHTS *****************//
/*
1 Sends out UDP to clients on repeat.
2 Clients join group.
3 Clients send a reply of messageVerification && their IP address
4 Server has struct array of clients
        struct array consists of most recent messageVerification && IP address
        have global counter tracking # of new clients?
5 Server verifies that it does not exist in array
6 update array with new group
7 spin off new multithreaded tcp connection.
8 utilize newMultiThreadFunc to verify state between client node and the server's local array for clients       state. i.e. the node number and most recent messageVerification
9 resend UDP packets until all states are verified through TCP
10. move on to next message.
*/
//****************************************//

int validateStates(){
        int i;
        if( numOfClients == 0){
                return 1; //means no states to validate
        }
        for( i =0; i < numOfClients; i++){
                if( clientMessageState[i].messageVerification != globalCounter)
                {
//                      printf("index %i CMSmessage %i != gc %i\n",i,clientMessageState[i].messageVerification, globalCounter);
                        return -1;
                }
        }
        return 1;
        //return 1 if all states are up to date with globalCounter
}

void *handle_clients(void *socke){
        int sock = (int)socke;
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
                               // clientMessageState[nodeStateIndex].upToDate = 1;
                clientMessageState[index].messageVerification = recvMessage.messageVerification;
//                printf("rcvd message num %i from node %i\n",recvMessage.messageVerification,recvMessage.nodeNumber);
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
        //fcntl(sock, F_SETFL, O_NONBLOCK);  // set to non-blocking
        if(sock < 0) {
                printf("socket error = %d\n", sock);
        }
        if(numOfClients == 0){
        i = bind(sock, (struct sockaddr*) &server, sizeof(server));
        if( i< 0) {
                printf("bind result: %d\n", i);
        }
        }
//      printf("socket bounded\n");
        //control statements
        if(listen(sock, 10)<0){
                printf("Listen Error\n");
        }
        while((i_socket = accept(sock,(struct sockaddr*)&addr,&addrlen))){
                int new_i;

                tRet = pthread_create(&threads[threadCount], NULL, handle_clients, (void *)i_socket);
                threadCount++;
                numOfClients++;
        }
}

int main(int argc, char *argv[])
{
        int sock, nbytes, flags, addrlen, size, i, n, iPort;
        struct sockaddr_in server, from;
        char group[12], port[6], buffer[500];
        u_char ttl;
        globalPort = 54000;
        if(argc != 2)
        {
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
        while(1)
        {
                message.groupNumber = 10;
                message.messageNumber = globalCounter;
                message.port = globalPort;
                message.attempt = attemptNumber;
                usleep(100000);//To ensure some variables update
                char ip[20];
                strcpy(message.ipaddress, "10.1.2.3");
                //message.ipaddress = ip;

                size = sendto(sock, &message, sizeof(message), flags, (struct sockaddr *) &server, sizeof(server));
                if(size < 0)
                {
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
