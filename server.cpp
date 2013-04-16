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
#include <string>
#include <map>
using namespace std;
#define MAX_RETRY 3
#define SERV_PORT 3000

struct Request{
	int sock_fd;
	struct sockaddr_in addr_client;
	int client_len;
	string data;
};

void splitData(const char* req, const char** data, int* no){
	*data = req + 16;
	int d = 0;
	for (int i = 0; i < 16; i++){
		d *= 10;
		d += (req[i] - '0');
	}
	*no = d;
}
map<int, string> dataStorage;
map<int, int> requestNumber;
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
		perror("socket error");
		exit(1);
	}
	//初始化服务器断地址
	memset(&addr_serv,0,sizeof(struct sockaddr_in));
	addr_serv.sin_family = AF_INET;//协议族
	addr_serv.sin_port = htons(SERV_PORT);
	addr_serv.sin_addr.s_addr = htonl(INADDR_ANY);//任意本地址
	Request* request;
	client_len = sizeof(struct sockaddr_in);
	if(bind(sock_fd,(struct sockaddr *)&addr_serv,sizeof(struct sockaddr_in))<0 ){
		perror("bind error");
		exit(1);
	}
	printf("server is ready:\n");
	pthread_attr_t attr;
	while (1){
		recv_num = recvfrom(sock_fd,recv_buf,sizeof(recv_buf),0,(struct sockaddr *)&addr_client,(socklen_t *)&client_len);
		printf("receive a request!!\n");
		if(recv_num >=  0){
			recv_buf[recv_num]='\0';
			request = new Request();
			request->sock_fd = sock_fd;
			request->addr_client = addr_client;
			request->client_len = client_len;
			request->data = recv_buf;
			pthread_t thread; 

			pthread_attr_init(&attr);
			//设置线程为分离属性
			pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
			pthread_create(&thread,&attr,&replyClient,(void *)request); 
			pthread_attr_destroy (&attr);
		}
	}
	pthread_exit(NULL);
	close(sock_fd);
	return 0;
}
void *replyClient(void* args){
	Request* request = (Request*) args;
	int retryCounter = 0;
	char bcBuf[1000];
	char sendBuf[1000];
	char temp[2000];
	while(true){
		if (request->data.length() > 548){
			break;
		}
		int ID;
		const char* p;
		splitData(request->data.c_str(), &p, &ID);
		if (dataStorage.find(ID) != dataStorage.end()){
			printf("from cache!!!\n");
			string out = dataStorage[ID];
			sendto(request->sock_fd,out.c_str(),out.length(),0,(struct sockaddr *)&(request->addr_client), request->client_len);
			break;
		}
		if (requestNumber.find(ID) != requestNumber.end()){
			printf("request is running, abandon");
			break;
		}
		requestNumber[ID] = 1;

		//Start to calculate
		sprintf(temp, "echo \"%s\" | bc", p);
		FILE *fp = popen(temp, "r");
		fscanf(fp, "%s", bcBuf);
		sprintf(sendBuf, "%016u%s", ID, bcBuf);
		dataStorage[ID] = string(sendBuf);
		// It's doesn't matter if send is failed
		sendto(request->sock_fd,sendBuf,strlen(sendBuf),0,(struct sockaddr *)&(request->addr_client), request->client_len);
		break;
	}
	delete request;
}
