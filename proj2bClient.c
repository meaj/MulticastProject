#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/unistd.h>
#include <stdlib.h>
#include <errno.h>
//Daniel Greenlees
//Kevin Moore

#include <sys/fcntl.h>
#include <pthread.h>
#define TARGET_PORT 22210
#define TARGET_IP  "239.10.5.10"

struct toClientPackage{
        int groupNumber;
        int messageNumber;
        int port;
        char ipaddress[20];
        int attempt;
};

struct toServerPackage{
        int ipaddress;
        int messageVerification;
        int nodeNumber;
};
//**** GOBAL CLIENT VARIABLES *******//
int multiThreadEnabled; 
struct toServerPackage stateOfClient;
struct toClientPackage udpIncoming;
int lastMessageReceived;
int sndrcvLock;

void *sndInfo(void *threadID){
        printf("CLIENT MULTITHREAD IS UP!\n");
//      uint32_t target_port = udpIncoming.port;
        struct sockaddr_in target;
        int sock, i;
//      int ipaddr = atoi(udpIncoming.ipaddress);
        printf("sndInfoStuff: %i %i %s\n", udpIncoming.messageNumber, udpIncoming.port, udpIncoming.ipaddress);
        target.sin_family = AF_INET;
        target.sin_port = htons(udpIncoming.port);
        target.sin_addr.s_addr = inet_addr(udpIncoming.ipaddress);
        sock = socket(AF_INET,SOCK_STREAM,0);
        if(sock < 0) {
                printf("TCP socket error = %d\n", sock);
        }
        i = connect(sock, (struct sockaddr *)&target , sizeof(target));
        if(i < 0){
                printf("Connection With Host Failed %d",i);
        }
        else{
                //TCP CODE GOES HERE
                //SETUP LOCK while whileloop goes
                while(1){
                        if(sndrcvLock == 1){
                                sndrcvLock = 0;
                                int size;
                                size = send(sock, (struct toServerPackage *)&stateOfClient, sizeof(stateOfClient), 0);
                                if(size < 0){
                                        printf("messaging send error %d\n",size);
                                }
                        //      printf("Sending %i stateOfClient to Server\n",stateOfClient.messageVerification);
                        }
                }
        }


        //Go through TCP connection
        //Establish Connection with Server TCP
        //send server state (toServerPackage struct) when new UDP comes in. 
}




int main(int argc, char * argv[]){

       time_t r_time;
//        struct tm* l_time;
        struct sockaddr_in server;
        int i, sock, addrlen, size;
        struct ip_mreq join_group;
        char buffer[50];
        multiThreadEnabled = 0; // 0 means not threaded
        memset(&server,0,sizeof(server));
        server.sin_family = PF_INET;
        server.sin_addr.s_addr= htonl(INADDR_ANY);
        server.sin_port = htons(TARGET_PORT);

//        time(&r_time);

        // setup socket
        if ((sock=socket(AF_INET, SOCK_DGRAM,0))<0){
                printf("socket error = %d\n", sock);
                return -1;
        }

        // bind socket
        i = bind(sock, (struct sockaddr*) &server, sizeof(server));
        if( i< 0) {
                printf("Main bind result: %d\n", i);
                return -1;
        }
                                               
        // send join request
        join_group.imr_multiaddr.s_addr=inet_addr(TARGET_IP);
    join_group.imr_interface.s_addr=htonl(INADDR_ANY);
    if (setsockopt(sock,IPPROTO_IP,IP_ADD_MEMBERSHIP,&join_group,sizeof(join_group)) < 0) {
                printf("Group join failed with error %d/n", errno);
                return -1;
    }
    printf("Group join succeded, waiting for broadcast/n");
        //Enabling TCP send thread;


        int flag = 0;
        // infinite loop to listen
        while (1){
                struct toClientPackage message;

                 if ((size = recvfrom(sock,&message,sizeof(message),0,(struct sockaddr*)&server,
                         &addrlen)) > 0)
                        {
                        if(stateOfClient.messageVerification != message.messageNumber){

                                printf("Received message number: %i", message.messageNumber);
                                if(message.attempt > 1){
                                        printf(" on attempt number: %i",message.attempt);
                                }
                                printf("\n");
                                udpIncoming = message;
                                lastMessageReceived = message.messageNumber;
                                stateOfClient.messageVerification = lastMessageReceived;
                                flag = message.messageNumber;
                                sndrcvLock = 1;
                                if(multiThreadEnabled == 0){
                                        //enables thread to send
                                        //Ensure globals are set before thread = 1;
                                        multiThreadEnabled = 1;
                                       //update state of server
                                        stateOfClient.messageVerification = message.messageNumber;
                                        stateOfClient.nodeNumber = 2;
                                        int tRet;
                                        pthread_t threads[2];
                                        tRet = pthread_create(&threads[0], NULL, sndInfo, (void *)0);

                                        //Start TCP Connect/send to Server
                                }
                                else{
                                        //already threaded
                                }
                        }
                        else{
                                flag++;
                        }

                }
        }
        close(sock);
}
