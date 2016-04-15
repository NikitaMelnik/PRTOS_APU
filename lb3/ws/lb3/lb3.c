#include <stdlib.h>
#include <stdio.h>

struct MESSAGE // ������������ ��������� send message structure
{
	unsigned char type; // 0,1,2,3,4,5,6
	unsigned int buf; // send data
};

int main(int argc, char *argv[]) {
	int status;
	struct MESSAGE msg; // ����� ����������� ���������
	unsigned char rmsg; // ����� ��������� ���������
	char command[10]; // ����� ������ ���������
	int coid;

	coid=name_open("apu/roby",0);
	printf("apu/roby has coid=%d\n\n", coid);

	do {
		printf("\n>");
		scanf("%s", command);

		switch (command[0]) {
		case 'A':       //Write port A
			msg.type = 0;
			sscanf(command + 1, "%X", &msg.buf);
			status = MsgSend(coid, &msg, sizeof(msg), &rmsg, sizeof(rmsg));
			printf("Write port A: message <0x%X>\n", msg.buf);
			break;

		case 'C':       //Write port C
			msg.type = 1;
			sscanf(command + 1, "%X", &msg.buf);
			status = MsgSend(coid, &msg, sizeof(msg), &rmsg, sizeof(rmsg));
			printf("Write port C: message <0x%X>\n", msg.buf);
			break;
		case 'c':       //Read port  C
			msg.type = 2;
			status = MsgSend(coid, &msg, sizeof(msg), &rmsg, sizeof(rmsg));
			printf("Port C: 0x%X\n", rmsg);
			break;
		case 'b':       //Read port  B
			msg.type = 3;
			status = MsgSend(coid, &msg, sizeof(msg), &rmsg, sizeof(rmsg));
			printf("Port B: 0x%X\n", rmsg);
			break;

		case 'E':
			return 0;       //Return from main
		default:
			printf("Unknown command\n");
			break;
		}
	} while (1);

	return EXIT_SUCCESS;
}
