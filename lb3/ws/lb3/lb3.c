#include <stdlib.h>
#include <stdio.h>

struct MESSAGE // ������������ ��������� send message structure
{
 unsigned char      type; // 0,1,2,3,4,5,6
 unsigned int       buf;// send data
};


int main(int argc, char *argv[]) {
	int status;
	    struct MESSAGE msg; // ����� ����������� ���������
	    unsigned char rmsg; // ����� ��������� ���������
		char command[10]; // ����� ������ ���������
	    int coid;

	//...........�������� ���������
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

	//�� �������� ���������
	//int MsgSend( int coid,
	//             const void* smsg,
	//             int sbytes,
	//             void* rmsg,
	//             int rbytes );
	          MsgSend(coid,);
	          break;

	        case 'C':       //Write port C
	          msg.type=??;
	//..... �������� ���������
	          break;
	        case 'c':       //Read port  C
	          msg.type=??;
	// ...... �������� ���������
	          break;
	        case 'b':       //Read port  B
	          msg.type=??;
	//........ �������� ���������
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
