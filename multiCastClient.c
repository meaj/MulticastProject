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

int main(int argc, char * argv[]){

        time_t r_time;
        struct tm* l_time;
        struct sockaddr_in server;
        int i, sock, addrlen, size;
        struct ip_mreq join_group;
        char buffer[50];

        memset(&server,0,sizeof(server));
        server.sin_family = PF_INET;
        server.sin_addr.s_addr= htonl(INADDR_ANY);
        server.sin_port = htons(TARGET_PORT);

        time(&r_time);

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

        // infinite loop to listen
        while (1){
                // If a message is received on the socket, print message and the current time
                if ((size = recvfrom(sock,&buffer,sizeof(buffer),0,(struct sockaddr*)&server, &addrlen)) > 0){
  //                      l_time = localtime(&r_time);
                struct tm* l_time;
                time(&r_time);
                l_time = localtime(&r_time);

                    printf("[%d-%d-%d %d:%d:%d] %s\n", l_time->tm_mday, l_time->tm_mon+1, l_time->tm_year+1900, l_time->tm_hour, l_time->tm_min, l_time->tm_sec, buffer);
                }
        }        close(sock);
}
