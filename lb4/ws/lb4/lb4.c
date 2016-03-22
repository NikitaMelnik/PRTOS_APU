#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
struct MESSAGE // пересылаемое сообщение send message structure
{
	unsigned char type; // 0,1,2,3,4,5,6
	unsigned int buf; // send data
};
void * Display(void *arg)
{
	ChannelCreate();
	printf("Channel number id=%d\n\n", );
	printf("\n");
}

int main(int argc, char *argv[]) {
	int status;
	struct MESSAGE msg; // буфер посылаемого сообщения
	unsigned char rmsg; // Буфер ответного сообщения
	char command[10]; // Буфер команд оператора
	int coid;

	coid=name_open("apu/roby",0);
	printf("apu/roby has coid=%d\n\n", coid);
	pthread_create( NULL, &attr, &Display, NULL );
