#include <ncurses.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <termios.h>
#include <unistd.h>
#include <sys/iofunc.h>
#include <sys/dispatch.h>
#include <sys/neutrino.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/siginfo.h>
#include "roby.h"
#include <errno.h>

#define ATTACH_POINT "apu/roby"
#define TIMER_W_UNSET 4
#define TIMER_F_UNSET 8
#define TIMER_W_COUNT 33
#define TIMER_F_COUNT 65
#define SIZE 12
#define DEFAULT_PULSE 66

int chid_XYZ, chid_WF;	//�������������� ������
int coid_roby, coid_WF;

//�������������� ����������
int status, status_WF;	//������ ��������� ���������
int state_W, state_F, w_cnt, f_cnt;	//state_W - �������� ��� W (1/-1) �������������
									//����������� ��������, w_cnt - �������� ���������� �� W
int main_pulse_priority;	//��������� ��������, ����������� ������ (��� W/F)
int xx, yy, zz;			//�������� ��������� �� x, y, z

int stop_w, stop_f;	//������ ��������� �� W, F
int stop_done;//������ ���������� ��������� (1-7)(��� �������� ���� ��������� ��������)
int stop_sensor;	//������ ���������� ������ �������� (��� ��������� �-���)

int prog_start;	//������ ������� ��������� (������/��� ����� ��������)

int x_value, y_value, z_value;	//�������� ��������
int x_old, y_old, z_old;		//����������
int w_value, f_value, w_old, f_old;

int xx_old, yy_old, zz_old;		//���������� (��� �-��� ��������)
int ww_old, ff_old;

struct _pulse Pulse;	//��������� ��������� ��������
struct _itimer itime;	//��������� ��������� �������

int stateW, stateF;
int chid_XYZWF;        // ����� ����� � ������� DisplayXYZWF.
int MConAtt, state_W, state_F, coid_roby;
int main_coid;         // ������������� �������� main().
int chid_SWF;          // ����� ����� � ������� SensorWF.
int chid_TWF;          // ����� ����� � ������� TimerWF.
int chid_main, number, x_cnt, y_cnt, z_cnt, w_cnt, f_cnt, s_cnt, d_cnt;
unsigned char DisplayState = 0; //����� ����������� ��������� ��������:
//0 - ��������; 1 - �������.
char complete_command[] = "Completed commands:\r\n\r\n\t";
int main_pulse_priority = 10; //��������� ���������, ����������� �� ������ main
//� ������ DisplayWF.

int ActiveW = 0, ActiveF = 0; //�������� ���������� W � F: ���������� ActiveW ��� ActiveF
//��������������� � 1, ����� ����� ��������� ������ Roby ������ ��������
//�� ���������� W ��� F ��������������.

struct _itimer itime_F;                   // ��������� ��������� ��� ��������.
struct _itimer itime_W;

struct MESSAGE                            // ��������� ��� ���������.
{
	unsigned char type;
	unsigned int buf[4];
};
struct MESSAGE msg;

struct PULSE1                              // ��������� ��� ���������.
{
	_Uint16t type;
	_Uint16t subtype;
	_Int8t code;
	_Uint8t value[3];
	union sigval zero;
	_Int32t scoid;
};

struct PULSE {
	char type;
	int value[3];
};

void func_stop() {
	int i;
	struct MESSAGE msg;

	for (i = 0; i < 3; i++) {
		msg.type = i;
		msg.buf[0] = 0;
		MsgSend(coid_roby, &msg, sizeof(msg), NULL, 0); // ���������� ������� ������.
	}

}      //func_stop

/* �-��� ������ ��������� �� X Y Z */
void *DisplayXYZ() {
	int rcvd;      //������������� ��������� ���������
	if (prog_start == 1)      //���� 1-� ������, �� ������ ��������� ��������
			{
		xx = yy = zz = 512;
		w_cnt = f_cnt = 32;
	};
	xx = x_old;      //���������� ������������ ���������
	yy = y_old;      //��� ����������� ��������� ����� ���������
	zz = z_old;      //�� �������
	w_cnt = w_old;
	f_cnt = f_old;
	chid_XYZ = ChannelCreate(0);      //������� �����
	msg.buf[0] = chid_XYZ;
	printw("Channel XYZ namber: <%d>\n", msg.buf);
	msg.type = 4;
	printw("Init X...\n");
	status = MsgSend(coid_roby, &msg, sizeof(msg), NULL, 0);
	msg.type = 5;
	printw("Init Y...\n");
	status = MsgSend(coid_roby, &msg, sizeof(msg), NULL, 0);
	msg.type = 6;
	printw("Init Z...\n");
	status = MsgSend(coid_roby, &msg, sizeof(msg), NULL, 0);
	while (1) {
		rcvd = MsgReceivePulse(chid_XYZ, &Pulse, sizeof(Pulse), NULL);
		//����� ����������� ���������
		//printw("PL: <%d>\n",rcvd);
		switch (Pulse.code)		//��������� ��� ��������
		{
		case 1:		//�������� ���������� �
			xx = Pulse.value.sival_int;
			break;

		case 2:
			yy = Pulse.value.sival_int;
			break;

		case 4:
			zz = Pulse.value.sival_int;
			break;

		default:
			break;
		}
		printw("X=%d Y=%d Z=%d W=%d F=%d S=0 D=0\n", xx, yy, zz, w_cnt, f_cnt);
	}
	ChannelDestroy(chid_XYZ);		//���������� ��������� �����
	pthread_exit(NULL);	//���������� ��������� �����
}
;

/* �-��� ������ ��������� �� W F */
void *DisplayWF() {
	int rcvd;	//������������� ��������� ���������
	int coid_W, coid_F;		//�������������� ���������
	int timer_W, timer_F;	//�������������� �������
	int print_w, print_f;	//�������������� ������ �� W F
	if (prog_start == 1)		//���� 1-� ������, �� ������ ��������� ��������
			{
		w_cnt = f_cnt = 32;
	};
	xx = x_old;		//���������� ���������� ����-�
	yy = y_old;
	zz = z_old;
	w_cnt = w_old;
	f_cnt = f_old;
	chid_WF = ChannelCreate(0);		//������� �����
	msg.buf[0] = chid_WF;
	printw("Channel WF namber: <%d>\n", msg.buf);
	pid_t PID;
	PID = getpid();		//�������� ����������� ID ��������
	coid_W = ConnectAttach(0, PID, chid_WF, 0, 0);		//������� ����������
	coid_F = ConnectAttach(0, PID, chid_WF, 0, 0);
	struct sigevent event_W;		//���������� ��������� �������
	struct sigevent event_F;
	//���������� �������� ��� �������� (SIGEV_PULSE_INIT)
	SIGEV_PULSE_INIT(&event_W, coid_W, SIGEV_PULSE_PRIO_INHERIT, TIMER_W_COUNT,
			NULL);
	SIGEV_PULSE_INIT(&event_F, coid_F, SIGEV_PULSE_PRIO_INHERIT, TIMER_F_COUNT,
			NULL);
	timer_W = TimerCreate(CLOCK_REALTIME, &event_W);		//�������� �������
	timer_F = TimerCreate(CLOCK_REALTIME, &event_F);
	msg.type = 7;
	printw("Init W...\n");
	status = MsgSend(coid_roby, &msg, sizeof(msg), NULL, 0);
	msg.type = 8;
	printw("Init F...\n");
	status = MsgSend(coid_roby, &msg, sizeof(msg), NULL, 0);
	while (1) {
		rcvd = MsgReceivePulse(chid_WF, &Pulse, sizeof(Pulse), NULL);
		//����� ����������� ���������
		//printw("STAT: <%d>\t",rcvd);
		//printw("Code: <%d>\t",Pulse.code);
		//printw("Value: <%d>\n",Pulse.value.sival_int);
		switch (Pulse.code)		//��������� ��� ��������
		{
		case 1:		//���� ������ ��������� ������ �� W
			itime.nsec = 5000000;		//����� 1-�� ������������ �������
			itime.interval_nsec = 160000000;		//�������� �������
			TimerSettime(timer_W, NULL, &itime, NULL);		//������ �������
			break;

		case 33:		//"�����" �� W
			if (stop_w == 1)		//���� ��������� ������� �� ���������
					{			 //� ������ ����������
				itime.nsec = 0;			 //������������� ������
				itime.interval_nsec = 0;
				TimerSettime(timer_W, NULL, &itime, NULL);
				printw("Code: <%d> Value: <%d>\n", Pulse.code,
						Pulse.value.sival_int);
				stop_w = 0;			 //�������� ������ ��������� �� W
				print_w = 1;			 //������ ������ �� W=1 ����� ��������
			}			  //�� ������� ���� � �� �� ������
			else			  //���� ������� ��������� �����������
			{	//������� ���������� �� W
				w_cnt = w_cnt + state_W;
			}
			;
			break;

		case 8:	//���� ����� �� W �� �����
			itime.nsec = 0;
			itime.interval_nsec = 0;
			TimerSettime(timer_W, NULL, &itime, NULL);
			printw("------------------------\n");
			printw("Code: <%d> Value: <%d>\n", Pulse.code,
					Pulse.value.sival_int);
			break;

		case 16:	//���� ����� �� W �� ������
			itime.nsec = 0;
			itime.interval_nsec = 0;
			TimerSettime(timer_W, NULL, &itime, NULL);
			printw("------------------------\n");
			printw("Code: <%d> Value: <%d>\n", Pulse.code,
					Pulse.value.sival_int);
			break;

		case 2:	//���� ������� ��������� ������ �� F
			itime.nsec = 5000000;
			itime.interval_nsec = 170000000;
			TimerSettime(timer_F, NULL, &itime, NULL);
			break;

		case 65:	//"�����" �� F
			if (stop_f == 1) {
				itime.nsec = 0;
				itime.interval_nsec = 0;
				TimerSettime(timer_F, NULL, &itime, NULL);
				printw("Code: <%d> Value: <%d>\n", Pulse.code,
						Pulse.value.sival_int);
				stop_f = 0;
				print_f = 1;
			} else {
				f_cnt = f_cnt + state_F;
			}
			;
			break;

		case 32:	//���� �� ����� �� F
			itime.nsec = 0;
			itime.interval_nsec = 0;
			TimerSettime(timer_F, NULL, &itime, NULL);
			printw("------------------------\n");
			printw("Code: <%d> Value: <%d>\n", Pulse.code,
					Pulse.value.sival_int);
			break;

		case 64:	//���� ����� �� ������ �� F
			itime.nsec = 0;
			itime.interval_nsec = 0;
			TimerSettime(timer_F, NULL, &itime, NULL);
			printw("------------------------\n");
			printw("Code: <%d> Value: <%d>\n", Pulse.code,
					Pulse.value.sival_int);
			break;

		default:
			break;
		};
		//������� ���� �� ��������� ������ ������
		//(�������� ���������� ������ ����� � ��� �� ������)
		if ((print_w != 1) && (print_f != 1)) {
			printw("X=%d Y=%d Z=%d W=%d F=%d S=0 D=0\n", xx, yy, zz, w_cnt,
					f_cnt);
		};
		//�������� ������� ������
		if (print_w == 1) {
			print_w = 0;
		};
		if (print_f == 1) {
			print_f = 0;
		};
	}
	ConnectDetach(coid_W);	//���������� ����������
	ConnectDetach(coid_F);
	ChannelDestroy(chid_WF);	//���������� ��������� �����
	pthread_exit(NULL);	//���������� ��������� �����
}
/* �-��� �����/������ ������ ��������� */
void *PrintCoor() {
	int active, active_msg;	//������ �������� ����������, ������
	int s_value, d_value, s_old, d_old;	//�������� S D
	int blok_sd;		//������ ���������� S D
	char command[10];	//�������, ���������� ������
	unsigned char rmsg;	//������������ ���������
	pthread_t thread11, thread22;	//�������������� �������
	active = stop_done = 0;	//�������� �������� ����������, ������ ����������
	active_msg = 0;		//��������� ��� �� ����������
	main_pulse_priority = 10;		//��������� ��������
	if (prog_start == 1)		//���� 1-� ������ ���������
			{
		x_old = y_old = z_old = 512;
		w_old = f_old = 32;
		//s_old=d_old=0;
	};
	xx_old = x_old;		//��������� ���������� ��������
	yy_old = y_old;
	zz_old = z_old;
	ww_old = w_old;
	ff_old = f_old;
	//������� ����� ����� ��� ���������� � DisplayXYZ � DisplayWF
	pthread_create(&thread11, NULL, &DisplayXYZ, NULL);
	sleep(1);		//���� 1 ���. ����� ����������
	pthread_create(&thread22, NULL, &DisplayWF, NULL);
	sleep(1);
	pid_t PID;
	PID = getpid();		//�������� ���������� ID ��������
	coid_WF = ConnectAttach(0, PID, chid_WF, 0, 0);		//������ ����������
	//������������� �������� ������� ��� ���������� ������
	//PTHREAD_CANCEL_DISABLE - ������� pthread_cancel ����� ���������������
	//PTHREAD_CANCEL_ENABLE - ������� ����� ���������
	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
	while (stop_done < 7)	//���� �� �������� ��� ��������� ��������
	{
		if (active == 0)	//���� ����� ��� �� ��������� �������
				{
			printw("\nInput X, Y, Z, W, F, S, D:");
			scanf("%d", &x_value);
			scanf("%d", &y_value);
			scanf("%d", &z_value);
			scanf("%d", &w_value);
			scanf("%d", &f_value);
			scanf("%d", &s_value);
			scanf("%d", &d_value);
		};

		if (x_value != x_old)	//���� ����� � ������ �������� ��������
				{
			//���� ������� � � "�����" ������
			if (((active == 0) && (x_value > x_old))
					|| ((active == 1) && (x_value > x_old))) {//��������� �������� �� 1/7 (stop_done=1)
				active = stop_done = 1;	//�������� ���������� �
				if (active_msg == 0)	//���� ��� �� �������� �������
						{
					strcpy(command, "A04");
					msg.type = 0;
					sscanf(command + 1, "%X", &msg.buf);
					status = MsgSend(coid_roby, &msg, sizeof(msg), &rmsg,
							sizeof(rmsg));
					active_msg = 1;	//������� ��������
				};
				if (xx == x_value)	//���� ����� �� ����� ����������
						{			   //������������� ������ �� �
					active = 0;			   //��������� ���
					active_msg = 0;	//�������� ��� ������ ��������� � ��. �����������
					strcpy(command, "A0");
					msg.type = 0;
					sscanf(command + 1, "%X", &msg.buf);
					status = MsgSend(coid_roby, &msg, sizeof(msg), &rmsg,
							sizeof(rmsg));
					x_old = x_value;			//���������� ���������� ��������
				};
			};

			//���� ������� � � "�����" �����
			if (((active == 0) && (x_value < x_old))
					|| ((active == 1) && (x_value < x_old))) {//��������� �������� �� 1/7 (stop_done=1)
				active = stop_done = 1;	//�������� ���������� �
				if (active_msg == 0)	//���� ��� �� �������� �������
						{
					strcpy(command, "A08");
					msg.type = 0;
					sscanf(command + 1, "%X", &msg.buf);
					status = MsgSend(coid_roby, &msg, sizeof(msg), &rmsg,
							sizeof(rmsg));
					active_msg = 1;	//������� ��������
				};
				if (xx == x_value)	//���� ����� �� ������ ����������
						{			   //������������� ������ �� �
					active = 0;			   //��������� ���
					active_msg = 0;	//�������� ��� ������ ��������� � ��. �����������
					strcpy(command, "A0");
					msg.type = 0;
					sscanf(command + 1, "%X", &msg.buf);
					status = MsgSend(coid_roby, &msg, sizeof(msg), &rmsg,
							sizeof(rmsg));
					x_old = x_value;			//���������� ���������� ��������
				};
			};
		};

		if (y_value != y_old) {
			if (((active == 0) && (y_value > y_old))
					|| ((active == 2) && (y_value > y_old))) {
				active = stop_done = 2;
				if (active_msg == 0) {
					strcpy(command, "A80");
					msg.type = 0;
					sscanf(command + 1, "%X", &msg.buf);
					status = MsgSend(coid_roby, &msg, sizeof(msg), &rmsg,
							sizeof(rmsg));
					active_msg = 1;
				};
				if (yy == y_value) {
					active = 0;
					active_msg = 0;
					strcpy(command, "A0");
					msg.type = 0;
					sscanf(command + 1, "%X", &msg.buf);
					status = MsgSend(coid_roby, &msg, sizeof(msg), &rmsg,
							sizeof(rmsg));
					y_old = y_value;
				};
			};

			if (((active == 0) && (y_value < y_old))
					|| ((active == 2) && (y_value < y_old))) {
				active = stop_done = 2;
				if (active_msg == 0) {
					strcpy(command, "A40");
					msg.type = 0;
					sscanf(command + 1, "%X", &msg.buf);
					status = MsgSend(coid_roby, &msg, sizeof(msg), &rmsg,
							sizeof(rmsg));
					active_msg = 1;
				};
				if (yy == y_value) {
					active = 0;
					active_msg = 0;
					strcpy(command, "A0");
					msg.type = 0;
					sscanf(command + 1, "%X", &msg.buf);
					status = MsgSend(coid_roby, &msg, sizeof(msg), &rmsg,
							sizeof(rmsg));
					y_old = y_value;
				};
			};
		};

		if (z_value != z_old) {
			if (((active == 0) && (z_value > z_old))
					|| ((active == 3) && (z_value > z_old))) {
				active = stop_done = 3;
				if (active_msg == 0) {
					strcpy(command, "A20");
					msg.type = 0;
					sscanf(command + 1, "%X", &msg.buf);
					status = MsgSend(coid_roby, &msg, sizeof(msg), &rmsg,
							sizeof(rmsg));
					active_msg = 1;
				};
				if (zz == z_value) {
					active = 0;
					active_msg = 0;
					strcpy(command, "A0");
					msg.type = 0;
					sscanf(command + 1, "%X", &msg.buf);
					status = MsgSend(coid_roby, &msg, sizeof(msg), &rmsg,
							sizeof(rmsg));
					z_old = z_value;
				};
			};

			if (((active == 0) && (z_value < z_old))
					|| ((active == 3) && (z_value < z_old))) {
				active = stop_done = 3;
				if (active_msg == 0) {
					strcpy(command, "A10");
					msg.type = 0;
					sscanf(command + 1, "%X", &msg.buf);
					status = MsgSend(coid_roby, &msg, sizeof(msg), &rmsg,
							sizeof(rmsg));
					active_msg = 1;
				};
				if (zz == z_value) {
					active = 0;
					active_msg = 0;
					strcpy(command, "A0");
					msg.type = 0;
					sscanf(command + 1, "%X", &msg.buf);
					status = MsgSend(coid_roby, &msg, sizeof(msg), &rmsg,
							sizeof(rmsg));
					z_old = z_value;
				};
			};
		};

		if (w_value != w_old)			 //���� ����� � ������ �������� ��������
				{	//���� ������� W � "�����" ������
			if (((active == 0) && (w_value > w_old))
					|| ((active == 4) && (w_value > w_old))) {//��������� �������� �� 4/7 (stop_done=4)
				active = stop_done = 4;	//������� W
				if (active_msg == 0)	//���� ��� �� �������� ���������
						{
					strcpy(command, "C10");
					msg.type = 1;
					sscanf(command + 1, "%X", &msg.buf);
					state_W = 1;	//�.�. "�����" ������
					//�������� ���������� ��������� � ������ ������� �� W
					status_WF = MsgSendPulse(coid_WF, main_pulse_priority,
							TIMER_W_SET, NULL);
					status = MsgSend(coid_roby, &msg, sizeof(msg), &rmsg,
							sizeof(rmsg));
					active_msg = 1;						//��������� ��������
				};
				if (w_cnt == w_value)		//���� �������� ������ ����������
						{				  //������������� ������ �� W
					stop_w = 1;				  //������ ��������� �� W
					active = 0;				  //��������� ���
					active_msg = 0;				 //�������� ��� ������ ���������
					strcpy(command, "C0");
					msg.type = 1;
					sscanf(command + 1, "%X", &msg.buf);
					status = MsgSend(coid_roby, &msg, sizeof(msg), &rmsg,
							sizeof(rmsg));
					w_old = w_value;				  //���������� ����������
				};
			};

			//���� �������� W � "�����" �����
			if (((active == 0) && (w_value < w_old))
					|| ((active == 4) && (w_value < w_old))) {//��������� �������� �� 4/7 (stop_done=4)
				active = stop_done = 4;	//������� W
				if (active_msg == 0)	//���� �� �������� ���������
						{
					strcpy(command, "C20");
					msg.type = 1;
					sscanf(command + 1, "%X", &msg.buf);
					state_W = -1;	//�.�. "�����" �����
					//�������� ���������� ��������� � ������ ������� �� W
					status_WF = MsgSendPulse(coid_WF, main_pulse_priority,
							TIMER_W_SET, NULL);
					status = MsgSend(coid_roby, &msg, sizeof(msg), &rmsg,
							sizeof(rmsg));
					active_msg = 1;						//��������� ��������
				};
				if (w_cnt == w_value)			//���� ����� ������ ����������
						{				  //�������� ������ �� W
					stop_w = 1;				  //������ ��������� �� W
					active = 0;				  //��������� ���
					active_msg = 0;				 //�������� ��� ������ ���������
					strcpy(command, "C0");
					msg.type = 1;
					sscanf(command + 1, "%X", &msg.buf);
					status = MsgSend(coid_roby, &msg, sizeof(msg), &rmsg,
							sizeof(rmsg));
					w_old = w_value;			//���������� ���������� ��������
				};
			};
		};

		if (f_value != f_old) {
			if (((active == 0) && (f_value > f_old))
					|| ((active == 5) && (f_value > f_old))) {
				active = stop_done = 5;
				if (active_msg == 0) {
					strcpy(command, "C40");
					msg.type = 1;
					sscanf(command + 1, "%X", &msg.buf);
					state_F = 1;
					status_WF = MsgSendPulse(coid_WF, main_pulse_priority,
							TIMER_F_SET, NULL);
					status = MsgSend(coid_roby, &msg, sizeof(msg), &rmsg,
							sizeof(rmsg));
					active_msg = 1;
				};
				if (f_cnt == f_value) {
					stop_f = 1;
					active = 0;
					active_msg = 0;
					strcpy(command, "C0");
					msg.type = 1;
					sscanf(command + 1, "%X", &msg.buf);
					status = MsgSend(coid_roby, &msg, sizeof(msg), &rmsg,
							sizeof(rmsg));
					f_old = f_value;
				};
			};

			if (((active == 0) && (f_value < f_old))
					|| ((active == 5) && (f_value < f_old))) {
				active = stop_done = 5;
				if (active_msg == 0) {
					strcpy(command, "C80");
					msg.type = 1;
					sscanf(command + 1, "%X", &msg.buf);
					state_F = -1;
					status_WF = MsgSendPulse(coid_WF, main_pulse_priority,
							TIMER_F_SET, NULL);
					status = MsgSend(coid_roby, &msg, sizeof(msg), &rmsg,
							sizeof(rmsg));
					active_msg = 1;
				};
				if (f_cnt == f_value) {
					stop_f = 1;
					active = 0;
					active_msg = 0;
					strcpy(command, "C0");
					msg.type = 1;
					sscanf(command + 1, "%X", &msg.buf);
					status = MsgSend(coid_roby, &msg, sizeof(msg), &rmsg,
							sizeof(rmsg));
					f_old = f_value;
				};
			};
		};

		blok_sd = 0;		//���������� (����������� �� ��� ������ ������� 1)
		//if ���������� ���� ��� ��������� X Y Z W F
		if (((active == 0) && (s_value == 1)) && (blok_sd == 0)) {//��������� �������� �� 6/7 (stop_done=6)
			stop_done = 6;
			strcpy(command, "A02");
			msg.type = 0;
			sscanf(command + 1, "%X", &msg.buf);
			status = MsgSend(coid_roby, &msg, sizeof(msg), &rmsg, sizeof(rmsg));
			s_old = s_value;	//���������� ���������� ��������
			blok_sd = 1;		//��������� D �� �����
			printw("X=%d Y=%d Z=%d W=%d F=%d S=%d D=0\n", xx, yy, zz, w_cnt,
					f_cnt, s_value);
		}

		if (((active == 0) && (d_value == 1)) && (blok_sd == 0)) {
			stop_done = 7;
			strcpy(command, "A01");
			msg.type = 0;
			sscanf(command + 1, "%X", &msg.buf);
			status = MsgSend(coid_roby, &msg, sizeof(msg), &rmsg, sizeof(rmsg));
			d_old = d_value;
			blok_sd = 1;
			printw("X=%d Y=%d Z=%d W=%d F=%d S=0 D=%d\n", xx, yy, zz, w_cnt,
					f_cnt, d_value);
		}

		if (((active == 0) && (s_value == d_value == 1)) && (blok_sd == 0)) {
			stop_done = 7;
			strcpy(command, "A03");
			msg.type = 0;
			sscanf(command + 1, "%X", &msg.buf);
			status = MsgSend(coid_roby, &msg, sizeof(msg), &rmsg, sizeof(rmsg));
			s_old = d_old = d_value;
			blok_sd = 1;
			printw("X=%d Y=%d Z=%d W=%d F=%d S=%d D=%d\n", xx, yy, zz, w_cnt,
					f_cnt, s_value, d_value);
		};
		//���� ��������� ��� ��������
		if (stop_done == 7) {	//������� �� � ��������� ���������� �����
			printw("--------\n");
			printw("X=%d Y=%d Z=%d W=%d F=%d S=%d D=%d\n", xx, yy, zz, w_cnt,
					f_cnt, s_value, d_value);
			pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
		};
	};
	ConnectDetach(coid_WF);			//���������� ����������
	pthread_cancel(thread11);		//���������� �����
	pthread_join(thread11, NULL);	//���� ��������� ������
	printw("Done THREAD_XYZ.\n");
	pthread_cancel(thread22);
	pthread_join(thread22, NULL);
	printw("Done THREAD_WF.\n");
	pthread_exit(NULL);	//���������� ��������� �����
	return EXIT_SUCCESS;
}

/* �-��� ������ ��������� �������� */
void *PrintSensors() {
	int x_start, y_start, z_start;	//�������� ��������
	int w_start, w_end;
	int f_start, f_end;
	x_start = y_start = z_start = 0;	//������� ��� ������
	w_start = f_start = 0;
	w_end = f_end = 0;
	stop_sensor = 0;//������ ���������� ������ �������� (��� ��������� �-���)
	//printw("X=%d Y=%d Z=%d W=%d F=%d\n",x_value,y_value,z_value,w_value,f_value);
	//printw("OLD X=%d Y=%d Z=%d W=%d F=%d\n",xx_old,yy_old,zz_old,ww_old,ff_old);
	if (x_value != xx_old)	//���� ������������ �� �
			{
		x_start = 1;	//������ ���.
	};
	if (y_value != yy_old) {
		y_start = 1;
	};
	if (z_value != zz_old) {
		z_start = 1;
	};
	if ((w_value != ww_old) && (w_value > ww_old))	//���� "������" ������ �� W
			{
		w_end = 1;	//�������� ������ ���.
	};
	if ((w_value != ww_old) && (w_value < ww_old))	//���� �����
			{
		w_start = 1;	//��������� ���.
	};
	if ((f_value != ff_old) && (f_value > ff_old)) {
		f_end = 1;
	};
	if ((f_value != ff_old) && (f_value < ff_old)) {
		f_start = 1;
	};
	printw("X_start=%d Y_start=%d Z_start=%d\n", x_start, y_start, z_start);
	printw("W_start=%d W_end=%d   F_start=%d F_end=%d", w_start, w_end, f_start,
			f_end);
	stop_sensor = 1;	//��� �������� - ���������� �-���
	pthread_exit(NULL);	//���������� ��������� �����
	return EXIT_SUCCESS;
}

void *TimerWF() {
	int ConAtt1;
	int ConAtt2;
	int ConAtt3;
	int ConDisplayXYZWF;
	int print;
	int timer_W;
	int timer_F;
//   int i;
	struct PULSE1 pl;

	struct MESSAGE msg;
	struct sigevent event1;
	struct sigevent event2;

	chid_TWF = ChannelCreate(0);                // �������� ������ ��� ��������.

	ConAtt1 = ConnectAttach(0, 0, chid_TWF, 0, 0); // �������� ���������� ���������� � 1-�� ��������.
	ConAtt2 = ConnectAttach(0, 0, chid_TWF, 0, 0); // �������� ���������� ���������� �� 2-�� ��������.
	ConAtt3 = ConnectAttach(0, 0, chid_main, 0, 0);
	ConDisplayXYZWF = ConnectAttach(0, main_coid, chid_XYZWF, 0, 0); // �������� ���������� ���������� � ������� DisplayXYZWF.

	SIGEV_PULSE_INIT(&event1, ConAtt1, 1, 0, TIMER_W_COUNT); // ���������� ��������� sigevent ��� ������� ���������� W.
	SIGEV_PULSE_INIT(&event2, ConAtt2, 1, 0, TIMER_F_COUNT); // ���������� ��������� sigevent ��� ������� ���������� F.
	timer_W = TimerCreate(CLOCK_SOFTTIME, &event1); // �������� ������������ ������� ��� ���������� W.
	timer_F = TimerCreate(CLOCK_SOFTTIME, &event2); // �������� ������������ ������� ��� ���������� F.

	while (1) {
		print = 0;
		MsgReceivePulse(chid_TWF, &pl, sizeof(pl), NULL); // ����� ����������� ���������.

		switch (pl.value[2]) {
		case TIMER_W_SET:        // ���� ����� ��������� ������ �� ���������� W.
			msg.type = 1;
			if (number > w_cnt) {
				msg.buf[0] = C_W_FORWARD;
				state_W = 1;
			} else {
				msg.buf[0] = C_W_BACK;
				state_W = -1;
			}

			itime_W.nsec = 160000000;
			itime_W.interval_nsec = 160000000;

			TimerSettime(timer_W, NULL, &itime_W, NULL);

			MsgSend(coid_roby, &msg, sizeof(msg), NULL, 0);   // ������ �������.
			break;

		case TIMER_F_SET:
			// ���� ����� ��������� ������ �� ���������� F.
			msg.type = 1;
			if (number > f_cnt) {
				msg.buf[0] = C_F_FORWARD;
				state_F = 1;
			} else {
				msg.buf[0] = C_F_BACK;
				state_F = -1;
			}

			itime_F.nsec = 160000000;
			itime_F.interval_nsec = 160000000;

			TimerSettime(timer_F, NULL, &itime_F, NULL);
			MsgSend(coid_roby, &msg, sizeof(msg), NULL, 0);
			break;
			// ���� ����� ���������� ������ �� ���������� W.
		case TIMER_F_UNSET:
			itime_W.nsec = 0;
			itime_W.interval_nsec = 0;
			TimerSettime(timer_F, NULL, &itime_W, NULL);
			break;

		case TIMER_W_COUNT:               // ����������/���������� ���������� W.
			if (state_W == 1) {
				w_cnt++;
			} else {
				w_cnt--;
			}                                 //if
			if (w_cnt == number) {          // �� ��������� ���������� ��������.
				msg.type = 1;
				msg.buf[0] = 0;
				itime_W.nsec = 0;
				itime_W.interval_nsec = 0;
				TimerSettime(timer_W, NULL, &itime_W, NULL);
				MsgSend(coid_roby, &msg, sizeof(msg), NULL, 0);
				msg.type = '5';
				msg.buf[0] = 11;
				MsgSend(ConAtt3, &msg, sizeof(msg), NULL, 0);
			}
			print = 1;
			break;

		case TIMER_F_COUNT:               // ����������/���������� ���������� F.
			if (state_F == 1) {
				f_cnt++;
			} else {
				f_cnt--;
			}                                 //if
			if (f_cnt == number) {          // �� ��������� ���������� ��������.
				msg.type = 1;
				msg.buf[0] = 0;
				MsgSend(coid_roby, &msg, sizeof(msg), NULL, 0);
				itime_F.nsec = 0;
				itime_F.interval_nsec = 0;
				TimerSettime(timer_F, NULL, &itime_F, NULL);

				msg.type = '5';
				msg.buf[0] = 12;
				MsgSend(ConAtt3, &msg, sizeof(msg), NULL, 0);
			}
			print = 1;
			break;
		}                              //switch

		if (print) {
			MsgSendPulse(ConDisplayXYZWF, 1, 0, DEFAULT_PULSE); // �������� ������ DisplayXYZWF �� ��������� ��������� WF.
		}    //if
//      sched_yield();

	}    //while
}    //TimerWF

void *SensorWF() {
	int i;
	struct PULSE pl;
	struct MESSAGE msg;

	chid_SWF = ChannelCreate(0); // �������� ������ ��� ��������� �������� �������� ���������.

	for (i = 7; i < 9; i++) {            //������������� �������� ��������� F,W
		msg.type = i;
		msg.buf[0] = chid_SWF;
		MsgSend(coid_roby, &msg, sizeof(msg), NULL, 0);
	};

	while (1) {
		MsgReceivePulse(chid_SWF, &pl, sizeof(pl), NULL); // ����� ����������� ���������.
		switch (pl.value[0]) {
		case W_END:
			w_cnt = 100;
			printw("The end of coordinates W, the robot is been stopped.\r\n");
			func_stop();
			break;

		case W_BEGIN:
			w_cnt = 0;
			printw(
					"The start of coordinates W, the robot is been stopped.\r\n");
			func_stop();
			break;

		case F_END:
			f_cnt = 50;
			printw("The end of coordinates F, the robot is been stopped.\r\n");
			func_stop();
			break;

		case F_BEGIN:
			f_cnt = 0;
			printw(
					"The start of coordinates F, the robot is been stopped.\r\n");
			func_stop();
			break;
		}
	}         //while
}         //SensorWF

int main(int argc, char *argv[]) {
	//initscr()
	initscr();
	printw("Start Work \n");
	unsigned char PA = 0, PC = 0; //��� ���������� ������ �������� ��������� PA � PC
	//������ Roby.
	int ch; //������, �������� ������������� � ����������.
	char Help[] =
			"Unknown command. Please, use next keys: <F1>-<F12>, <Enter>, <+/=> or <Esc> for exit."; //�������.
	char command; //��� �������.

	int status;
	struct MESSAGE msg; //���������� ���������.
	unsigned char rmsg; //����� ��������� ���������.
	//char command[10]; //����� ��� ������ ���������.

	int coid; //������������� ����������, ������� ���������� �������� ������ Roby.

	coid_roby = coid = name_open("apu/roby", NULL); //���������� coid (������������� ����������)
													//�� ����� "apu/roby".
	printw("apu/roby has coid=%d \n", coid);
	pthread_t thread33;	//������������� ������
	pthread_create(&thread33, NULL, &PrintCoor, NULL);

	//���������� � ������� chidWF (��� ������� ��������).
	sleep(5); //������������� ����� main, ����� ��������� ������� DisplayXYZWF
	//������������� � ������ (��������, ���������������� ������� �������� �� X, Y � Z
	//� ������� ������� ��������� �� W � F ��������� ������ roby).
//
//	/*****Syntax for ConnectAttach()*****************
//	 * int ConnectAttach(int nd, //Node Descriptor - ���������� ���� � ���� (nd=0, ����
//	 *				//��������� ����������);
//	 *		pid_t pid, //PID (Process ID);
//	 *		int chid, //CHID (Channel ID);
//	 *		unsigned index,
//	 *		int flags); //�����.
//	 ************************************************/
	printw("Pult1_var2: main thread: please, press any key.\n");
	do { //����������� ����.
		sleep(1); //�������� ���������� ������ DisplayXYZ ���
		//������ DisplayWF (�� ��������� �����).

		//��������� � �������������.
		ch = getch(); //getchar();//getch(); //������� ������, �������� � ����������.
		switch (ch) { //������ ASCII-���� ������� �������.
		case 27: //������ ������� <Esc>.
			printw("\n<Esc> - exit..."); //���������� ���������.
			getch(); //getch();
			command = 'E'; //��� �������.
			return 0;
		case 13: //������ ������� <Enter>.
			printw("<Enter> - move to start position on all coordinates"
					"\nand clear position values.");
			//��������� � ��������� ��������� �� ���� �����������
			//� �������� �������� �������� ���������.
			//command=...; //��� �������.
			PA = 0;
			PC = 0; //��� ����� �������� � 0 ���������� PA � PC.
			command = 'C'; //������ ������� ������ � ������� PC ������ Roby.
			break;
		case '+':
		case '=': //������ ������� <+/=>.
			//����������� ����� ����������� ��������� ��������.
			DisplayState ^= 0x01; //����������� ����������� ��� (XOR)
			//��� ���������� DisplayState ���������� � � 0
			//���� ������������� � 1.
			printw("\n<+/=> - display state = %d.", DisplayState);
			command = 'W';
			break;
		case 0: //������ ����������� ��� �������������� �������.
			switch (ch = getch()) { //��� ������� ��������������� �������.
			case 59: //������ ������� <F1>.
				//������/���������� �������� �� X �����.
				printw("\n<F1> - toggle moving on X forward.");
				PA ^= A_X_FORWARD; //����������� ��� A_X_FORWARD � �������� PA.
				command = 'A'; //��� �������.
				break;
			case 60: //������ ������� <F2>.
				//������/���������� �������� �� X �����.
				printw("\n<F2> - toggle moving on X backward.");
				PA ^= A_X_BACK; //����������� ��� A_X_BACK � �������� PA.
				command = 'A'; //��� �������.
				break;
			case 61: //������ ������� <F3>.
				//������/���������� �������� �� Y �����.
				printw("\n<F3> - toggle moving on Y forward.");
				PA ^= A_Y_FORWARD; //����������� ��� A_Y_FORWARD � �������� PA.
				command = 'A'; //��� �������.
				break;
			case 62: //������ ������� <F4>.
				//������/���������� �������� �� Y �����.
				printw("\n<F4> - toggle moving on Y backward.");
				PA ^= A_Y_BACK; //����������� ��� A_Y_BACK � �������� PA.
				command = 'A'; //��� �������.
				break;
			case 63: //������ ������� <F5>.
				//������/���������� �������� �� Z �����.
				printw("\n<F5> - toggle moving on Z forward.");
				PA ^= A_Z_FORWARD; //����������� ��� A_Z_FORWARD � �������� PA.
				command = 'A'; //��� �������.
				break;
			case 64: //������ ������� <F6>.
				//������/���������� �������� �� Z �����.
				printw("\n<F6> - toggle moving on Z backward.");
				PA ^= A_Z_BACK; //����������� ��� A_Z_BACK � �������� PA.
				command = 'A'; //��� �������.
				break;
			case 65: //������ ������� <F7>.
				//������/���������� �������� �� F �����.
				printw("\n<F7> - toggle rotating on F forward.");
				PC ^= C_F_FORWARD; //����������� ��� C_F_FORWARD � �������� PC.
				command = 'C'; //��� �������.
				ActiveF = 1; //F - �������� ����������.
				if (stateF == 1) //���� Roby �������� ����� �� F,
					stateF = 0; //�� ���� �� F.
				else
					//�����
					stateF = 1; //��������� �������� ����� �� F.
				break;
			case 66: //������ ������� <F8>.
				//������/���������� �������� �� F �����.
				printw("\n<F8> - toggle rotating on F backward.");
				PC ^= C_F_BACK; //����������� ��� C_F_BACK � �������� PC.
				command = 'C'; //��� �������.
				ActiveF = 1; //F - �������� ����������.
				if (stateF == -1) //���� Roby �������� ����� �� F,
					stateF = 0; //�� ���� �� F.
				else
					//�����
					stateF = -1; //��������� �������� ����� �� F.
				break;
			case 67: //������ ������� <F9>.
				//������/���������� �������� �� W �����.
				printw("\n<F9> - toggle rotating head on W forward.");
				PC ^= C_W_FORWARD; //����������� ��� C_W_FORWARD � �������� PC.
				command = 'C'; //��� �������.
				ActiveW = 1; //W - �������� ����������.
				if (stateW == 1) //���� Roby �������� ����� �� W,
					stateW = 0; //�� ���������� ��� �� W.
				else
					//�����
					stateW = 1; //��������� �������� ����� �� W.
				break;
			case 68: //������ ������� <F10>.
				//������/���������� �������� �� W �����.
				printw("\n<F10> - toggle rotating head on W backward.");
				PC ^= C_W_BACK; //����������� ��� C_W_BACK � �������� PC.
				command = 'C'; //��� �������.
				ActiveW = 1; //W - �������� ����������.
				if (stateW == -1) //���� Roby �������� ����� �� W,
					stateF = 0; //�� ���������� ��� �� W.
				else
					//�����
					stateF = -1; //��������� �������� ����� �� W.
				break;
			case 133: //������ ������� <F11>.
				//��������/��������� ����� S.
				printw("\n<F11> - toggle S.");
				PA ^= A_S; //����������� ��� A_S � �������� PA.
				command = 'A'; //��� �������.
				break;
			case 134: //������ ������� <F12>.
				//��������/��������� ����� D.
				printw("\n<F12> - toggle drill work.");
				PA ^= A_D; //����������� ��� A_D � �������� PA.
				command = 'A'; //��� �������.
				break;
			} //End of switch().
			break;
		default: //������ ������ �������.
			printw("\n%s", Help); //puts(Help);
			command = 'U'; //����������� ��� �������.
		} //End of switch().
		if (DisplayState == 1) { //���� ������� ����� ����������� ��������� ��������, ��
			//command='b'; //������� ������ �� ����� PB ������ Roby.
			command = 'c'; //������� ������ �� ����� PC ������ Roby.
		} //End of if().
		  //.....������ ���� ������� command.
		switch (command) { //������������� ��� �������, �������� �������� command.
		case 'A':       //Write port A.
			msg.type = 0; //��� ���������.
			msg.buf[0] = PA; //�������� � ����� ��������� msg.buf �������� ���������� PA.
			//sscanf(command+1,"%X", &msg.buf); //������������� ������ (command+1)
			//�� 16-������ ���� � ��������� ��������� � msg.buf.
			printw("Write port A.");
			status = MsgSend(coid, &msg, sizeof(msg), &rmsg, sizeof(rmsg));
			//int MsgSend( int coid, //������� ���������, ��� coid - ������������� ����������;
			//             const void* smsg, //smsg - ��������� �� ���������� ���������;
			//             int sbytes, //sbytes - ������ (� ������) ����������� ���������;
			//             void* rmsg, //rmsg - ��������� �� ����� ������������ ������ �� roby;
			//             int rbytes ); //rbytes - ������ (� ������) ������ ��� ������������ ������.
			break;

		case 'C':       //Write port C.
			msg.type = 1; //��� ���������.
			msg.buf[0] = PC; //�������� � ����� ��������� msg.buf �������� ���������� PC.
			//sscanf(command+1,"%X", &msg.buf); //������� 16-����� ������ �� (command+1)
			//� ����� msg.buf.
			printw("Write port C.");
			//.....������ ������� �������� ���������:
			//���������� ���������� �������� (W ��� F); (stateW ��� stateF ���
			//�����������) ������� ���������� ��������� ��� ������� ��������
			//������ DisplayWF, ��������� MsgSendPulse().

			/*****Syntax for MsgSendPulse()************************
			 * MsgSendPulse(int coid, //����������;
			 *		int priority, //��������� ��������;
			 *		char code, //��� (���) ��������;
			 *		int value); //��������.
			 ******************************************************/

			if (ActiveW) //���� ����� �������� ���������� W, ��
				MsgSendPulse(coid_WF, main_pulse_priority, TIMER_W_SET, NULL); //��������� ������ �� W.
			if (ActiveF) //���� ����� �������� ���������� F, ��
				MsgSendPulse(coid_WF, main_pulse_priority, TIMER_F_SET, NULL); //��������� ������ �� F.

			status = MsgSend(coid, &msg, sizeof(msg), &rmsg, sizeof(rmsg)); //�������
			//��������� msg � roby.
			break;

		case 'c':       //Read port C.
			msg.type = 2; //��� ���������.
			//����� ��������� msg.buf ��� ������ ����� C ��������� �� ���������.
			printw("Read port C.");
			status = MsgSend(coid, &msg, sizeof(msg), &rmsg, sizeof(rmsg)); //�������
			//��������� msg � roby � ������� ����� rmsg �� roby.
			printw("Port C: 0x%X", rmsg); //����� �� ����� (� ����� stdout) ����� rmsg
			//� 16-����� ����.
			break;

		case 'b':       //Read port B.
			msg.type = 3; //��� ���������.
			//����� ��������� msg.buf ��� ������ ����� B ��������� �� ���������.
			//sscanf(command+1,"%X", &msg.buf); //*(command+1) => msg.buf
			printw("Read port B.");
			status = MsgSend(coid, &msg, sizeof(msg), &rmsg, sizeof(rmsg)); //�������
			//��������� msg � roby � ������� ����� rmsg �� roby.
			printw("Port B: 0x%X", rmsg); //����� �� ����� (� ����� stdout) ����� rmsg
			//� 16-����� ����.
			break;
		case 'W': //����� ������, ��� �������� ��� ������
			break;
		case 'E': //����� �� ���������.
			//endwin();
			return 0;       //Return from main().

		default:	//������ ������.
			printw("Unknown command\n");
			break;
		}
	} while (1);
	printw("Stop Work \n");
	getch();
	endwin();
	return EXIT_SUCCESS;
}
