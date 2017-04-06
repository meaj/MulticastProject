#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/unistd.h>
#include <stdlib.h>
//Daniel Greenlees

#include <sys/fcntl.h>
#include <pthread.h>
#define TARGET_PORT 22201
#define TARGET_IP  67.11.114.117
#define LOCAL_IP 127.0.0.1
//*******SETUP*******//
struct udpPackage{
	struct timeval clockTimer;
	int index;
	int totalSent;
};
struct udpPackage udpSendPacket;
struct udpPackage udpRecievePacket;
struct timeval clock1Start,clock2Start,clock1End,clock2End;
int numPacketsToSend = 5000; //number of packets program will send
int globalSocket; // global socket
struct timeval compBuff[5000];
//*******************//


void *udpClient(void *threadID){
	int sock;
	struct sockaddr_in target;
	char buffer[50];
	
	target.sin_family = AF_INET;
	target.sin_port = htons(TARGET_PORT);
	
	
	//target.sin_addr.s_addr = inet_addr("127.0.0.1");
	target.sin_addr.s_addr = inet_addr("67.11.114.117");
	
	for(int i = 0; i <numPacketsToSend; i++){
		gettimeofday(&clock1Start, NULL);
		compBuff[i] = clock1Start;
		sprintf(buffer, "%d", i);
		int size = sendto(globalSocket,&buffer,sizeof(buffer),0,(struct sockaddr*)&target, sizeof(target));
		if(size < 0){
			printf("messaging error %d\n",size);
		}
			usleep(1000);
	}
	//usleep(100000);
	
}



int main(int argc, char * argv[]){
	int tRet;
	long seconds, nanosec, miliseconds;
	struct sockaddr_storage addr;
	struct timeval rtt_timer;
	int i, addrlen, size;
	char buffer[50];

	struct sockaddr_in server;
	server.sin_family = PF_INET;
	server.sin_addr.s_addr= htonl(INADDR_ANY);
	server.sin_port = htons(22200);
	
	globalSocket = socket(AF_INET,SOCK_DGRAM,0);
	if(globalSocket < 0) {
		printf("socket error = %d\n", globalSocket);
		return -1;
	}
	
	i = bind(globalSocket, (struct sockaddr*) &server, sizeof(server));
	if( i< 0) {
		printf("Main bind result: %d\n", i);
		return -1;
	}
	//timout 5 seconds no incoming data
	rtt_timer.tv_sec = 5;
	rtt_timer.tv_usec = 0;
	if(setsockopt(globalSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&rtt_timer, sizeof(struct timeval)) < 0){
		printf("setsockopt Error");
		return -1;
	}
	addrlen = sizeof(addr);
	
	int count = 0;
	long totalRTT=0;
	//Create thread
	pthread_t threads[2];
	tRet = pthread_create(&threads[0], NULL, udpClient, (void *)0);
	if (tRet){
		printf("thread error = %d\n", tRet);
	return -1;
	}
	usleep(5000);
	

	while( (size = recvfrom(globalSocket,&buffer,sizeof(buffer),0,(struct sockaddr*)&addr, &addrlen)) != 0){
		if(size == -1){
			break;
		}
		int i = atoi (buffer);
		gettimeofday(&clock1End, NULL);
		seconds = clock1End.tv_sec - compBuff[i].tv_sec;
		nanosec = clock1End.tv_usec - compBuff[i].tv_usec;
		miliseconds = (seconds*1000 + nanosec/1000.0);
		printf("packet# %s, rtt in miliseconds == %ld\n", buffer,miliseconds);
		totalRTT += miliseconds;
		count++;
	}
	printf("%i/%i packets recieved, %f/1 packet loss\n",count,numPacketsToSend,(1 - (double)count / (double)numPacketsToSend));
	double dtotalRTT = totalRTT;
	if(count == 0){
	count++;
	}
	printf("Total RTT: %f ms, AVG RTT: %f ms\n",dtotalRTT, dtotalRTT/count);
	
	usleep(200000);
	close(globalSocket);
}