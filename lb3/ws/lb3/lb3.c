#include <stdlib.h>
#include <stdio.h>

struct MESSAGE // пересылаемое сообщение send message structure
{
 unsigned char      type; // 0,1,2,3,4,5,6
 unsigned int       buf;// send data
};


int main(int argc, char *argv[]) {
	int status;
	    struct MESSAGE msg; // буфер посылаемого сообщения
	    unsigned char rmsg; // Буфер ответного сообщения
		char command[10]; // Буфер команд оператора
	    int coid;

	//...........Вставьте операторы
	//int name_open( const char * name,
	//               int flags );
		printf("apu/roby has coid=%d\n\n",coid);

	    do {
	      printf("\n>");
	      scanf("%s",command);

		  switch(command[0]) {
	        case 'A':       //Write port A
	          msg.type=0;
	          sscanf(command+1,"%X",&msg.buf);

	//…… Вставьте операторы
	//int MsgSend( int coid,
	//             const void* smsg,
	//             int sbytes,
	//             void* rmsg,
	//             int rbytes );
	          MsgSend(coid,);
	          break;

	        case 'C':       //Write port C
	          msg.type=??;
	//..... Вставьте операторы
	          break;
	        case 'c':       //Read port  C
	          msg.type=??;
	// ...... Вставьте операторы
	          break;
	        case 'b':       //Read port  B
	          msg.type=??;
	//........ Вставьте операторы
	          break;
	        case 'E':
	          return 0;       //Return from main
	        default:
			  printf("Unknown command\n");
	        break;
	      }
	    } while(1);

	return EXIT_SUCCESS;
}
