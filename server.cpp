#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <vector>
#include <string>
using namespace std;
#define MAX_RETRY 3

struct Request{
	int sock_fd;
	struct sockaddr_in addr_client;
	int client_len;
	string data;
};

#define SERV_PORT 3000
void* replyClient(void* args);
int main(int argc, const char *argv[])
{
	int sock_fd;   //套接子描述符号
	int recv_num;
	int send_num;
	int client_len;
	char recv_buf[1000];
	struct sockaddr_in addr_serv;
	struct sockaddr_in addr_client;//服务器和客户端地址
	sock_fd = socket(AF_INET,SOCK_DGRAM,0);
	if(sock_fd < 0){
		perror("socket");
		exit(1);
	} else{
		printf("sock sucessful\n");
	}
	//初始化服务器断地址
	memset(&addr_serv,0,sizeof(struct sockaddr_in));
	addr_serv.sin_family = AF_INET;//协议族
	addr_serv.sin_port = htons(SERV_PORT);
	addr_serv.sin_addr.s_addr = htonl(INADDR_ANY);//任意本地址
	Request* request;
	client_len = sizeof(struct sockaddr_in);
	/*绑定套接子*/
	if(bind(sock_fd,(struct sockaddr *)&addr_serv,sizeof(struct sockaddr_in))<0 ){
		perror("bind");
		exit(1);
	} else{
		printf("bind sucess\n");
	}
	while (1){
		printf("server is ready:\n");
		recv_num = recvfrom(sock_fd,recv_buf,sizeof(recv_buf),0,(struct sockaddr *)&addr_client,(socklen_t *)&client_len);
		printf("rece\n");
		if(recv_num >=  0){
			recv_buf[recv_num]='\0';
			request = new Request();
			request->sock_fd = sock_fd;
			request->addr_client = addr_client;
			request->client_len = client_len;
			request->data = recv_buf;
			replyClient(request);
			pthread_t thread; 

			pthread_attr_t attr;
			pthread_attr_init(&attr);
			//pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
			///*  设置线程为分离属性*/ 
			pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
			int t;	
			t = pthread_create(&thread,&attr,&replyClient,(void *)request); 
			printf("%d\n", t);
			printf("pleaseTT");
			pthread_attr_destroy (&attr);
		}
		printf("loop end\n");
	}
	pthread_exit(NULL);
	close(sock_fd);
	return 0;
}
void *replyClient(void* args){
	Request* request = (Request*) args;
	int retryCounter = 0;
	char outputBuffer[1000];
	char temp[2000];
	sprintf(temp, "echo \"%s\" | bc", request->data.c_str());
	FILE *fp = popen(temp, "r");
	fscanf(fp, "%s", outputBuffer);
	int send_num = -1;
	while (send_num < 0 && retryCounter < MAX_RETRY){
		send_num = sendto(request->sock_fd,outputBuffer,strlen(outputBuffer),0,(struct sockaddr *)&(request->addr_client), request->client_len);
		retryCounter++;
	}
	delete request;
}
