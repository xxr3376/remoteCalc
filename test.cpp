/* example.c*/
#include <stdio.h>
#include <pthread.h>
#include <cstdlib>
using namespace std;
void *thread(void* )
{
	int i;
	for(i=0;i<100;i++)
		printf("This is a pthread.\n");
}

int main(void)
{
	pthread_t id;
	int i,ret;
	ret=pthread_create(&id,NULL,thread,NULL);
	if(ret!=0){
		printf ("Create pthread error!\n");
		exit (1);
	}
	for(i=0;i<100;i++)
		printf("This is the main process.\n");
	while(true){}
	return (0);
}

