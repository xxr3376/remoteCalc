#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <cstdlib>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <ctime>
#include <netinet/in.h>
using namespace std;

#define MAX_RETRY 10
#define DEST_PORT 3000
#define DSET_IP_ADDRESS  "127.0.0.1"

void splitData(char* req, char** data, int* no){
	*data = req + 16;
	int d = 0;
	for (int i = 0; i < 16; i++){
		d *= 10;
		d += (req[i] - '0');
	}
	*no = d;
}


int getID(){
	return rand()*rand()*rand();
}

char send_buf[548] = {};
char recv_buf[1000];

int CurID;

int main(int argc, const char *argv[])
{
	// input check
	srand(time(NULL));
	CurID = getID();
	if (argc == 2){
		if (strlen(argv[1]) > 500){
			printf("input is too long, please input a easier one\n");
			exit(-1);
		}
		sprintf(send_buf, "%016u%s", CurID, argv[1]);
	}
	else{
		sprintf(send_buf, "%016u1+1", CurID);
	}
	//INIT
	int sock_fd;/*套接字文件描述符*/
	int send_num;
	int recv_num;
	int dest_len;
	struct sockaddr_in addr_serv;/*服务端地址，客户端地址*/

	sock_fd = socket(AF_INET,SOCK_DGRAM,0);//创建套接子
	//初始化服务器端地址
	memset(&addr_serv,0,sizeof(addr_serv));
	addr_serv.sin_family = AF_INET;
	addr_serv.sin_addr.s_addr = inet_addr(DSET_IP_ADDRESS);
	addr_serv.sin_port = htons(DEST_PORT);
	dest_len = sizeof(struct sockaddr_in);
	send_num = -1;
	struct timeval tv;
	tv.tv_sec = 4;
	tv.tv_usec = 0;
	setsockopt(sock_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

	//Send
	int sendRetryCounter = 0;
	int waitRetryCounter = 0;
	while (true){
		send_num = sendto(sock_fd,send_buf,sizeof(send_buf),0,(struct sockaddr *)&addr_serv,dest_len);
		if(send_num < 0){
			fprintf(stderr, "Socket Fail");
			if (++sendRetryCounter > MAX_RETRY){
				fprintf(stderr, ", Already try 10 times, please check network\n");
				exit(-1);
			}
			fprintf(stderr, ", Retry to send\n");
			continue;
		}
		recv_num = recvfrom(sock_fd,recv_buf,sizeof(recv_buf),0,(struct sockaddr *)&addr_serv,(socklen_t *)&dest_len);
		if (recv_num < 0 ){
			if (errno == EWOULDBLOCK) {
				fprintf(stderr, "Server timeout");
				if (++waitRetryCounter > MAX_RETRY){
					fprintf(stderr, ", Already try 10 times, please check network or complex of your input");
					exit(-1);
				}
				fprintf(stderr,", Retry to send same request\n");
				continue;
			} else{
				fprintf(stderr, "Unknown Error\n");
				exit(-1);
			}
		}
		// Success
		
		recv_buf[recv_num]='\0';
		char* answer;
		int recvNo;
		splitData(recv_buf, &answer, &recvNo);
		if (CurID != recvNo){
			fprintf(stderr, "Wrong ID");
			if (++waitRetryCounter > MAX_RETRY){
				fprintf(stderr, ", Server responses illegally, Failed\n");
				exit(-1);
			}
			continue;
		}
		printf("the answer is :%s\n",answer);
		break;
	}
	close(sock_fd);
	return 0;
}

