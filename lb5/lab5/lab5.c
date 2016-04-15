#define W_END 0x08
#define W_BEGIN 0x10
#define F_END 0x20
#define F_BEGIN 0x40

#define TIMER_W_SET 1
#define TIMER_F_SET 2
#define TIMER_W_COUNT 33
#define TIMER_F_COUNT 65

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/iofunc.h>
#include <sys/dispatch.h>
#include <sys/neutrino.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/siginfo.h>
#include <fcntl.h>
#include <pthread.h>
#include <time.h>

int chid_XYZ, chid_WF;	//�������������� ������
int coid_roby, coid_WF;;//�������������� ����������
int status, status_WF;	//������ ��������� ���������
int state_W, state_F, w_cnt, f_cnt;//state_W - �������� ��� W (1/-1) �������������
								   //����������� ��������, w_cnt - �������� ���������� �� W
int main_pulse_priority;//��������� ��������, ����������� ������ (��� W/F)
int xx, yy, zz;			//�������� ��������� �� x, y, z

int stop_w, stop_f;	//������ ��������� �� W, F
int stop_done;		//������ ���������� ��������� (1-7)(��� �������� ���� ��������� ��������)
int stop_sensor;	//������ ���������� ������ �������� (��� ��������� �-���)

int prog_start;//������ ������� ��������� (������/��� ����� ��������)

int x_value, y_value, z_value;	//�������� ��������
int x_old, y_old, z_old;		//����������
int w_value, f_value, w_old, f_old;

int xx_old, yy_old, zz_old;//���������� (��� �-��� ��������)
int ww_old, ff_old;

struct _pulse Pulse;	//��������� ��������� ��������
struct _itimer itime;	//��������� ��������� �������

struct MESSAGE//��������� ��������� ���������
{
	unsigned char type;
	unsigned int buf;
};

struct MESSAGE msg;

/* �-��� ������ ��������� �� X Y Z */
void *DisplayXYZ()
{
	int rcvd;//������������� ��������� ���������
	if(prog_start==1)//���� 1-� ������, �� ������ ��������� ��������
	{
		xx=yy=zz=512;
		w_cnt=f_cnt=32;
	};
	xx=x_old;//���������� ������������ ���������
	yy=y_old;//��� ����������� ��������� ����� ���������
	zz=z_old;//�� �������
	w_cnt=w_old;
	f_cnt=f_old;
	chid_XYZ=ChannelCreate(0);//������� �����
	msg.buf=chid_XYZ;
	printf("Channel XYZ namber: <%d>\n",msg.buf);
	msg.type=4;
	printf("Init X...\n");
	status=MsgSend(coid_roby, &msg, sizeof(msg), NULL, 0);
	msg.type=5;
	printf("Init Y...\n");
	status=MsgSend(coid_roby, &msg, sizeof(msg), NULL, 0);
	msg.type=6;
	printf("Init Z...\n");
	status=MsgSend(coid_roby, &msg, sizeof(msg), NULL, 0);
	while(1)
	{
		rcvd=MsgReceivePulse(chid_XYZ, &Pulse, sizeof(Pulse), NULL);
		//����� ����������� ���������
		//printf("PL: <%d>\n",rcvd);
		switch(Pulse.code)//��������� ��� ��������
		{
			case 1://�������� ���������� �
			xx=Pulse.value.sival_int;
			break;

			case 2:
			yy=Pulse.value.sival_int;
			break;

			case 4:
			zz=Pulse.value.sival_int;
			break;

			default:
			break;
		}
		printf("X=%d Y=%d Z=%d W=%d F=%d S=0 D=0\n",xx,yy,zz,w_cnt,f_cnt);
	}
	ChannelDestroy(chid_XYZ);//���������� ��������� �����
	pthread_exit(NULL);	//���������� ��������� �����
};

/* �-��� ������ ��������� �� W F */
void *DisplayWF()
{
	int rcvd;//������������� ��������� ���������
	int coid_W, coid_F;		//�������������� ���������
	int timer_W, timer_F;	//�������������� �������
	int print_w, print_f;	//�������������� ������ �� W F
	unsigned char rmsg;		//������������ ���������
	if(prog_start==1)//���� 1-� ������, �� ������ ��������� ��������
	{
		w_cnt=f_cnt=32;
	};
	xx=x_old;//���������� ���������� ����-�
	yy=y_old;
	zz=z_old;
	w_cnt=w_old;
	f_cnt=f_old;
	chid_WF=ChannelCreate(0);//������� �����
	msg.buf=chid_WF;
	printf("Channel WF namber: <%d>\n",msg.buf);
	pid_t PID;
	PID=getpid();//�������� ����������� ID ��������
	coid_W=ConnectAttach(0,PID,chid_WF,0,0);//������� ����������
	coid_F=ConnectAttach(0,PID,chid_WF,0,0);
	struct sigevent event_W;//���������� ��������� �������
	struct sigevent event_F;
	//���������� �������� ��� �������� (SIGEV_PULSE_INIT)
	SIGEV_PULSE_INIT(&event_W, coid_W, SIGEV_PULSE_PRIO_INHERIT, TIMER_W_COUNT, NULL);
	SIGEV_PULSE_INIT(&event_F, coid_F, SIGEV_PULSE_PRIO_INHERIT, TIMER_F_COUNT, NULL);
	timer_W=TimerCreate(CLOCK_REALTIME, &event_W);//�������� �������
	timer_F=TimerCreate(CLOCK_REALTIME, &event_F);
	msg.type=7;
	printf("Init W...\n");
	status=MsgSend(coid_roby, &msg, sizeof(msg), NULL, 0);
	msg.type=8;
	printf("Init F...\n");
	status=MsgSend(coid_roby, &msg, sizeof(msg), NULL, 0);
	while(1)
	{
		rcvd=MsgReceivePulse(chid_WF, &Pulse, sizeof(Pulse), NULL);
		//����� ����������� ���������
		//printf("STAT: <%d>\t",rcvd);
		//printf("Code: <%d>\t",Pulse.code);
		//printf("Value: <%d>\n",Pulse.value.sival_int);
		switch(Pulse.code)//��������� ��� ��������
		{
			case 1://���� ������ ��������� ������ �� W
			itime.nsec=5000000;//����� 1-�� ������������ �������
			itime.interval_nsec=160000000;//�������� �������
			TimerSettime(timer_W, NULL, &itime, NULL);//������ �������
			break;

			case 33://"�����" �� W
			if(stop_w==1)//���� ��������� ������� �� ���������
			{			 //� ������ ����������
				itime.nsec=0;//������������� ������
				itime.interval_nsec=0;
				TimerSettime(timer_W, NULL, &itime, NULL);
				printf("Code: <%d> Value: <%d>\n",Pulse.code,Pulse.value.sival_int);
				stop_w=0;//�������� ������ ��������� �� W
				print_w=1;//������ ������ �� W=1 ����� ��������
			}			  //�� ������� ���� � �� �� ������
			else//���� ������� ��������� �����������
			{	//������� ���������� �� W
				w_cnt=w_cnt+state_W;
			};
			break;

			case 8://���� ����� �� W �� �����
			itime.nsec=0;
			itime.interval_nsec=0;
			TimerSettime(timer_W, NULL, &itime, NULL);
			printf("------------------------\n");
			printf("Code: <%d> Value: <%d>\n",Pulse.code,Pulse.value.sival_int);
			break;

			case 16://���� ����� �� W �� ������
			itime.nsec=0;
			itime.interval_nsec=0;
			TimerSettime(timer_W, NULL, &itime, NULL);
			printf("------------------------\n");
			printf("Code: <%d> Value: <%d>\n",Pulse.code,Pulse.value.sival_int);
			break;

			case 2://���� ������� ��������� ������ �� F
			itime.nsec=5000000;
			itime.interval_nsec=170000000;
			TimerSettime(timer_F, NULL, &itime, NULL);
			break;

			case 65://"�����" �� F
			if(stop_f==1)
			{
				itime.nsec=0;
				itime.interval_nsec=0;
				TimerSettime(timer_F, NULL, &itime, NULL);
				printf("Code: <%d> Value: <%d>\n",Pulse.code,Pulse.value.sival_int);
				stop_f=0;
				print_f=1;
			}
			else
			{
				f_cnt=f_cnt+state_F;
			};
			break;


			case 32://���� �� ����� �� F
			itime.nsec=0;
			itime.interval_nsec=0;
			TimerSettime(timer_F, NULL, &itime, NULL);
			printf("------------------------\n");
			printf("Code: <%d> Value: <%d>\n",Pulse.code,Pulse.value.sival_int);
			break;


			case 64://���� ����� �� ������ �� F
			itime.nsec=0;
			itime.interval_nsec=0;
			TimerSettime(timer_F, NULL, &itime, NULL);
			printf("------------------------\n");
			printf("Code: <%d> Value: <%d>\n",Pulse.code,Pulse.value.sival_int);
			break;

			default:
			break;
		};
		//������� ���� �� ��������� ������ ������
		//(�������� ���������� ������ ����� � ��� �� ������)
		if((print_w!=1)&&(print_f!=1))
		{
			printf("X=%d Y=%d Z=%d W=%d F=%d S=0 D=0\n",xx,yy,zz,w_cnt,f_cnt);
		};
		//�������� ������� ������
		if(print_w==1)
		{
			print_w=0;
		};
		if(print_f==1)
		{
			print_f=0;
		};
	}
	ConnectDetach(coid_W);	//���������� ����������
	ConnectDetach(coid_F);
	ChannelDestroy(chid_WF);//���������� ��������� �����
	pthread_exit(NULL);//���������� ��������� �����
};

/* �-��� �����/������ ������ ��������� */
void *PrintCoor()
{
	int active, active_msg;//������ �������� ����������, ������
	int s_value, d_value, s_old, d_old;//�������� S D
	int blok_sd;		//������ ���������� S D
	char command[10];	//�������, ���������� ������
	unsigned char rmsg;	//������������ ���������
	pthread_t thread11, thread22;//�������������� �������
	active=stop_done=0;	//�������� �������� ����������, ������ ����������
	active_msg=0;		//��������� ��� �� ����������
	main_pulse_priority=10;//��������� ��������
	if(prog_start==1)//���� 1-� ������ ���������
	{
		x_old=y_old=z_old=512;
		w_old=f_old=32;
		//s_old=d_old=0;
	};
	xx_old=x_old;//��������� ���������� ��������
	yy_old=y_old;
	zz_old=z_old;
	ww_old=w_old;
	ff_old=f_old;
	//������� ����� ����� ��� ���������� � DisplayXYZ � DisplayWF
	pthread_create(&thread11, NULL, &DisplayXYZ, NULL);
	sleep(1);//���� 1 ���. ����� ����������
	pthread_create(&thread22, NULL, &DisplayWF, NULL);
	sleep(1);
	pid_t PID;
	PID=getpid();//�������� ���������� ID ��������
	coid_WF=ConnectAttach(0,PID,chid_WF,0,0);//������ ����������
	//������������� �������� ������� ��� ���������� ������
	//PTHREAD_CANCEL_DISABLE - ������� pthread_cancel ����� ���������������
	//PTHREAD_CANCEL_ENABLE - ������� ����� ���������
	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
	while(stop_done<7)//���� �� �������� ��� ��������� ��������
	{
			if(active==0)//���� ����� ��� �� ��������� �������
			{
				printf("\nInput X, Y, Z, W, F, S, D:");
				scanf("%d", &x_value);
				scanf("%d", &y_value);
				scanf("%d", &z_value);
				scanf("%d", &w_value);
				scanf("%d", &f_value);
				scanf("%d", &s_value);
				scanf("%d", &d_value);
			};

			if(x_value!=x_old)//���� ����� � ������ �������� ��������
			{
				//���� ������� � � "�����" ������
				if(((active==0)&&(x_value>x_old))||((active==1)&&(x_value>x_old)))
				{	//��������� �������� �� 1/7 (stop_done=1)
					active=stop_done=1;//�������� ���������� �
					if(active_msg==0)//���� ��� �� �������� �������
					{
						strcpy(command,"A04");
						msg.type=0;
						sscanf(command+1, "%X", &msg.buf);
						status=MsgSend(coid_roby, &msg, sizeof(msg), &rmsg, sizeof(rmsg));
						active_msg=1;//������� ��������
					};
					if(xx==x_value)//���� ����� �� ����� ����������
					{			   //������������� ������ �� �
						active=0;//��������� ���
						active_msg=0;//�������� ��� ������ ��������� � ��. �����������
						strcpy(command,"A0");
						msg.type=0;
						sscanf(command+1,"%X",&msg.buf);
						status=MsgSend(coid_roby, &msg, sizeof(msg), &rmsg, sizeof(rmsg));
						x_old=x_value;//���������� ���������� ��������
					};
				};

				//���� ������� � � "�����" �����
				if(((active==0)&&(x_value<x_old))||((active==1)&&(x_value<x_old)))
				{	//��������� �������� �� 1/7 (stop_done=1)
					active=stop_done=1;//�������� ���������� �
					if(active_msg==0)//���� ��� �� �������� �������
					{
						strcpy(command,"A08");
						msg.type=0;
						sscanf(command+1, "%X", &msg.buf);
						status=MsgSend(coid_roby, &msg, sizeof(msg), &rmsg, sizeof(rmsg));
						active_msg=1;//������� ��������
					};
					if(xx==x_value)//���� ����� �� ������ ����������
					{			   //������������� ������ �� �
						active=0;//��������� ���
						active_msg=0;//�������� ��� ������ ��������� � ��. �����������
						strcpy(command,"A0");
						msg.type=0;
						sscanf(command+1,"%X",&msg.buf);
						status=MsgSend(coid_roby, &msg, sizeof(msg), &rmsg, sizeof(rmsg));
						x_old=x_value;//���������� ���������� ��������
					};
				};
			};

			if(y_value!=y_old)
			{
				if(((active==0)&&(y_value>y_old))||((active==2)&&(y_value>y_old)))
				{
					active=stop_done=2;
					if(active_msg==0)
					{
						strcpy(command,"A80");
						msg.type=0;
						sscanf(command+1, "%X", &msg.buf);
						status=MsgSend(coid_roby, &msg, sizeof(msg), &rmsg, sizeof(rmsg));
						active_msg=1;
					};
					if(yy==y_value)
					{
						active=0;
						active_msg=0;
						strcpy(command,"A0");
						msg.type=0;
						sscanf(command+1,"%X",&msg.buf);
						status=MsgSend(coid_roby, &msg, sizeof(msg), &rmsg, sizeof(rmsg));
						y_old=y_value;
					};
				};

				if(((active==0)&&(y_value<y_old))||((active==2)&&(y_value<y_old)))
				{
					active=stop_done=2;
					if(active_msg==0)
					{
						strcpy(command,"A40");
						msg.type=0;
						sscanf(command+1, "%X", &msg.buf);
						status=MsgSend(coid_roby, &msg, sizeof(msg), &rmsg, sizeof(rmsg));
						active_msg=1;
					};
					if(yy==y_value)
					{
						active=0;
						active_msg=0;
						strcpy(command,"A0");
						msg.type=0;
						sscanf(command+1,"%X",&msg.buf);
						status=MsgSend(coid_roby, &msg, sizeof(msg), &rmsg, sizeof(rmsg));
						y_old=y_value;
					};
				};
			};

			if(z_value!=z_old)
			{
				if(((active==0)&&(z_value>z_old))||((active==3)&&(z_value>z_old)))
				{
					active=stop_done=3;
					if(active_msg==0)
					{
						strcpy(command,"A20");
						msg.type=0;
						sscanf(command+1, "%X", &msg.buf);
						status=MsgSend(coid_roby, &msg, sizeof(msg), &rmsg, sizeof(rmsg));
						active_msg=1;
					};
					if(zz==z_value)
					{
						active=0;
						active_msg=0;
						strcpy(command,"A0");
						msg.type=0;
						sscanf(command+1,"%X",&msg.buf);
						status=MsgSend(coid_roby, &msg, sizeof(msg), &rmsg, sizeof(rmsg));
						z_old=z_value;
					};
				};

				if(((active==0)&&(z_value<z_old))||((active==3)&&(z_value<z_old)))
				{
					active=stop_done=3;
					if(active_msg==0)
					{
						strcpy(command,"A10");
						msg.type=0;
						sscanf(command+1, "%X", &msg.buf);
						status=MsgSend(coid_roby, &msg, sizeof(msg), &rmsg, sizeof(rmsg));
						active_msg=1;
					};
					if(zz==z_value)
					{
						active=0;
						active_msg=0;
						strcpy(command,"A0");
						msg.type=0;
						sscanf(command+1,"%X",&msg.buf);
						status=MsgSend(coid_roby, &msg, sizeof(msg), &rmsg, sizeof(rmsg));
						z_old=z_value;
					};
				};
			};

			if(w_value!=w_old)//���� ����� � ������ �������� ��������
			{	//���� ������� W � "�����" ������
				if(((active==0)&&(w_value>w_old))||((active==4)&&(w_value>w_old)))
				{	//��������� �������� �� 4/7 (stop_done=4)
					active=stop_done=4;//������� W
					if(active_msg==0)//���� ��� �� �������� ���������
					{
						strcpy(command,"C10");
						msg.type=1;
						sscanf(command+1, "%X", &msg.buf);
						state_W=1;//�.�. "�����" ������
						//�������� ���������� ��������� � ������ ������� �� W
						status_WF=MsgSendPulse(coid_WF, main_pulse_priority, TIMER_W_SET, NULL);
						status=MsgSend(coid_roby, &msg, sizeof(msg), &rmsg, sizeof(rmsg));
						active_msg=1;//��������� ��������
					};
					if(w_cnt==w_value)//���� �������� ������ ����������
					{				  //������������� ������ �� W
						stop_w=1;//������ ��������� �� W
						active=0;//��������� ���
						active_msg=0;//�������� ��� ������ ���������
						strcpy(command,"C0");
						msg.type=1;
						sscanf(command+1,"%X",&msg.buf);
						status=MsgSend(coid_roby, &msg, sizeof(msg), &rmsg, sizeof(rmsg));
						w_old=w_value;//���������� ����������
					};
				};

				//���� �������� W � "�����" �����
				if(((active==0)&&(w_value<w_old))||((active==4)&&(w_value<w_old)))
				{	//��������� �������� �� 4/7 (stop_done=4)
					active=stop_done=4;//������� W
					if(active_msg==0)//���� �� �������� ���������
					{
						strcpy(command,"C20");
						msg.type=1;
						sscanf(command+1, "%X", &msg.buf);
						state_W=-1;//�.�. "�����" �����
						//�������� ���������� ��������� � ������ ������� �� W
						status_WF=MsgSendPulse(coid_WF, main_pulse_priority, TIMER_W_SET, NULL);
						status=MsgSend(coid_roby, &msg, sizeof(msg), &rmsg, sizeof(rmsg));
						active_msg=1;//��������� ��������
					};
					if(w_cnt==w_value)//���� ����� ������ ����������
					{				  //�������� ������ �� W
						stop_w=1;//������ ��������� �� W
						active=0;//��������� ���
						active_msg=0;//�������� ��� ������ ���������
						strcpy(command,"C0");
						msg.type=1;
						sscanf(command+1,"%X",&msg.buf);
						status=MsgSend(coid_roby, &msg, sizeof(msg), &rmsg, sizeof(rmsg));
						w_old=w_value;//���������� ���������� ��������
					};
				};
			};

			if(f_value!=f_old)
			{
				if(((active==0)&&(f_value>f_old))||((active==5)&&(f_value>f_old)))
				{
					active=stop_done=5;
					if(active_msg==0)
					{
						strcpy(command,"C40");
						msg.type=1;
						sscanf(command+1, "%X", &msg.buf);
						state_F=1;
						status_WF=MsgSendPulse(coid_WF, main_pulse_priority, TIMER_F_SET, NULL);
						status=MsgSend(coid_roby, &msg, sizeof(msg), &rmsg, sizeof(rmsg));
						active_msg=1;
					};
					if(f_cnt==f_value)
					{
						stop_f=1;
						active=0;
						active_msg=0;
						strcpy(command,"C0");
						msg.type=1;
						sscanf(command+1,"%X",&msg.buf);
						status=MsgSend(coid_roby, &msg, sizeof(msg), &rmsg, sizeof(rmsg));
						f_old=f_value;
					};
				};

				if(((active==0)&&(f_value<f_old))||((active==5)&&(f_value<f_old)))
				{
					active=stop_done=5;
					if(active_msg==0)
					{
						strcpy(command,"C80");
						msg.type=1;
						sscanf(command+1, "%X", &msg.buf);
						state_F=-1;
						status_WF=MsgSendPulse(coid_WF, main_pulse_priority, TIMER_F_SET, NULL);
						status=MsgSend(coid_roby, &msg, sizeof(msg), &rmsg, sizeof(rmsg));
						active_msg=1;
					};
					if(f_cnt==f_value)
					{
						stop_f=1;
						active=0;
						active_msg=0;
						strcpy(command,"C0");
						msg.type=1;
						sscanf(command+1,"%X",&msg.buf);
						status=MsgSend(coid_roby, &msg, sizeof(msg), &rmsg, sizeof(rmsg));
						f_old=f_value;
					};
				};
			};

			blok_sd=0;//���������� (����������� �� ��� ������ ������� 1)
			//if ���������� ���� ��� ��������� X Y Z W F
			if(((active==0)&&(s_value==1))&&(blok_sd==0))
			{	//��������� �������� �� 6/7 (stop_done=6)
				stop_done=6;
				strcpy(command,"A02");
				msg.type=0;
				sscanf(command+1, "%X", &msg.buf);
				status=MsgSend(coid_roby, &msg, sizeof(msg), &rmsg, sizeof(rmsg));
				s_old=s_value;	//���������� ���������� ��������
				blok_sd=1;		//��������� D �� �����
				printf("X=%d Y=%d Z=%d W=%d F=%d S=%d D=0\n",xx,yy,zz,w_cnt,f_cnt,s_value);
			}

			if(((active==0)&&(d_value==1))&&(blok_sd==0))
			{
				stop_done=7;
				strcpy(command,"A01");
				msg.type=0;
				sscanf(command+1, "%X", &msg.buf);
				status=MsgSend(coid_roby, &msg, sizeof(msg), &rmsg, sizeof(rmsg));
				d_old=d_value;
				blok_sd=1;
				printf("X=%d Y=%d Z=%d W=%d F=%d S=0 D=%d\n",xx,yy,zz,w_cnt,f_cnt,d_value);
			}

			if(((active==0)&&(s_value==d_value==1))&&(blok_sd==0))
			{
				stop_done=7;
				strcpy(command,"A03");
				msg.type=0;
				sscanf(command+1, "%X", &msg.buf);
				status=MsgSend(coid_roby, &msg, sizeof(msg), &rmsg, sizeof(rmsg));
				s_old=d_old=d_value;
				blok_sd=1;
				printf("X=%d Y=%d Z=%d W=%d F=%d S=%d D=%d\n",xx,yy,zz,w_cnt,f_cnt,s_value,d_value);
			};
			//���� ��������� ��� ��������
			if(stop_done==7)
			{	//������� �� � ��������� ���������� �����
				printf("--------\n");
				printf("X=%d Y=%d Z=%d W=%d F=%d S=%d D=%d\n",xx,yy,zz,w_cnt,f_cnt,s_value,d_value);
				pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
			};
	};
	ConnectDetach(coid_WF);			//���������� ����������
	pthread_cancel(thread11);		//���������� �����
	pthread_join(thread11, NULL);	//���� ��������� ������
	printf("Done THREAD_XYZ.\n");
	pthread_cancel(thread22);
	pthread_join(thread22, NULL);
	printf("Done THREAD_WF.\n");
	pthread_exit(NULL);//���������� ��������� �����
	return EXIT_SUCCESS;
};

/* �-��� ������ ��������� �������� */
void *PrintSensors()
{
	int x_start, y_start, z_start;//�������� ��������
	int w_start, w_end;
	int f_start, f_end;
	x_start=y_start=z_start=0;//������� ��� ������
	w_start=f_start=0;
	w_end=f_end=0;
	stop_sensor=0;//������ ���������� ������ �������� (��� ��������� �-���)
	//printf("X=%d Y=%d Z=%d W=%d F=%d\n",x_value,y_value,z_value,w_value,f_value);
	//printf("OLD X=%d Y=%d Z=%d W=%d F=%d\n",xx_old,yy_old,zz_old,ww_old,ff_old);
	if(x_value!=xx_old)//���� ������������ �� �
	{
		x_start=1;//������ ���.
	};
	if(y_value!=yy_old)
	{
		y_start=1;
	};
	if(z_value!=zz_old)
	{
		z_start=1;
	};
	if((w_value!=ww_old)&&(w_value>ww_old))//���� "������" ������ �� W
	{
		w_end=1;//�������� ������ ���.
	};
	if((w_value!=ww_old)&&(w_value<ww_old))//���� �����
	{
		w_start=1;//��������� ���.
	};
	if((f_value!=ff_old)&&(f_value>ff_old))
	{
		f_end=1;
	};
	if((f_value!=ff_old)&&(f_value<ff_old))
	{
		f_start=1;
	};
	printf("X_start=%d Y_start=%d Z_start=%d\n",x_start,y_start,z_start);
	printf("W_start=%d W_end=%d   F_start=%d F_end=%d",w_start,w_end,f_start,f_end);
	stop_sensor=1;//��� �������� - ���������� �-���
	pthread_exit(NULL);//���������� ��������� �����
	return EXIT_SUCCESS;
};

main()
{
	char command[10];		//�������� ������
	unsigned char rmsg;	//������������ ���������
	prog_start=0;		//������ ����� ���������
	coid_roby=name_open("apu/roby",0);
	printf("apu/roby has coid=%d\n\n",coid_roby);
	pthread_t thread33;	//������������� ������
	while(1)
	{
		if (prog_start==0)
				{
					prog_start++; //first run
					pthread_create(&thread33, NULL, &PrintCoor, NULL);
				}
		sleep(2);
		printf("\nInput command:>");
		scanf("%s",command);
		switch(command[0])//��������� ��������� ������
		{


			case'c'://������ ����� �
			msg.type=2;
			sscanf(command+1,"%X",&msg.buf);
			status=MsgSend(coid_roby, &msg, sizeof(msg), &rmsg, sizeof(rmsg));
			printf("Read port c: message <%s>\n",&rmsg);
			break;

			case'b'://������ ����� B
			msg.type=3;
			sscanf(command+1,"%X",&msg.buf);
			status=MsgSend(coid_roby, &msg, sizeof(msg), &rmsg, sizeof(rmsg));
			printf("Read port b: message <%s>\n",&rmsg);
			break;

			case 'D'://����� ��������� ��������
			if(prog_start==0)//���� ������ ������ � �� ������� ����������
			{
				xx_old=yy_old=zz_old=512;
				ww_old=ff_old=32;
				x_value=y_value=z_value=512;
				w_value=f_value=32;
			};
			pthread_create(&thread33, NULL, &PrintSensors, NULL);
			break;

			case 'E'://����� �� ���������
			return 0;
			default:
			printf("Unknown command\n");
			break;
		};

		if(stop_done==7)//���� ��������� ��� ����������
		{				//������������� �����
			pthread_cancel(thread33);
			pthread_join(thread33, NULL);
		};
		if(stop_sensor==1)//���� ������ ������ ��������
		{				  //������������� �����
			pthread_cancel(thread33);
			pthread_join(thread33, NULL);
		};
	};
}
