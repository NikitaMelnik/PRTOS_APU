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

int chid_XYZ, chid_WF;	//идентификаторы канала
int coid_roby, coid_WF;

//идентификаторы соединения
int status, status_WF;	//статус ответного сообщения
int state_W, state_F, w_cnt, f_cnt;	//state_W - значение для W (1/-1) соответствуют
									//направлению движения, w_cnt - значение координаты по W
int main_pulse_priority;	//приоритет импульса, посылаемого роботу (для W/F)
int xx, yy, zz;			//значения координат по x, y, z

int stop_w, stop_f;	//статус остановки по W, F
int stop_done;//статус выполнения программы (1-7)(для проверки всех введенных значений)
int stop_sensor;	//статус выполнения вывода датчиков (для остановки ф-ции)

int prog_start;	//статус запуска программы (первый/уже давно работает)

int x_value, y_value, z_value;	//вводимые значения
int x_old, y_old, z_old;		//предыдущие
int w_value, f_value, w_old, f_old;

int xx_old, yy_old, zz_old;		//предыдущие (для ф-ции Датчиков)
int ww_old, ff_old;

struct _pulse Pulse;	//объявляем структуру импульса
struct _itimer itime;	//объявляем структуру таймера

int stateW, stateF;
int chid_XYZWF;        // Канал связи с потоком DisplayXYZWF.
int MConAtt, state_W, state_F, coid_roby;
int main_coid;         // Идентификатор процесса main().
int chid_SWF;          // Канал связи с потоком SensorWF.
int chid_TWF;          // Канал связи с потоком TimerWF.
int chid_main, number, x_cnt, y_cnt, z_cnt, w_cnt, f_cnt, s_cnt, d_cnt;
unsigned char DisplayState = 0; //Режим отображения состояния датчиков:
//0 - выключен; 1 - включен.
char complete_command[] = "Completed commands:\r\n\r\n\t";
int main_pulse_priority = 10; //Приоритет импульсов, поступающих от потока main
//к потоку DisplayWF.

int ActiveW = 0, ActiveF = 0; //Активные координаты W и F: переменная ActiveW или ActiveF
//устанавливается в 1, когда нужно заставить робота Roby начать движение
//по координате W или F соответственно.

struct _itimer itime_F;                   // Временные структуры для таймеров.
struct _itimer itime_W;

struct MESSAGE                            // Структура для сообщений.
{
	unsigned char type;
	unsigned int buf[4];
};
struct MESSAGE msg;

struct PULSE1                              // Структура для импульсов.
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
		MsgSend(coid_roby, &msg, sizeof(msg), NULL, 0); // Остановить команды робота.
	}

}      //func_stop

/* Ф-ция вывода координат по X Y Z */
void *DisplayXYZ() {
	int rcvd;      //идентификатор принятого сообщения
	if (prog_start == 1)      //если 1-й запуск, то задаем начальные значения
			{
		xx = yy = zz = 512;
		w_cnt = f_cnt = 32;
	};
	xx = x_old;      //предыдущие приравниваем начальным
	yy = y_old;      //для дальнейшего сравнения новых введенных
	zz = z_old;      //со старыми
	w_cnt = w_old;
	f_cnt = f_old;
	chid_XYZ = ChannelCreate(0);      //создаем канал
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
		//прием импульсного сообщения
		//printw("PL: <%d>\n",rcvd);
		switch (Pulse.code)		//проверяем код импульса
		{
		case 1:		//меняется координата х
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
	ChannelDestroy(chid_XYZ);		//уничтожаем созданный канал
	pthread_exit(NULL);	//уничтожаем созданный поток
}
;

/* Ф-ция вывода координат по W F */
void *DisplayWF() {
	int rcvd;	//идентификатор принятого сообщения
	int coid_W, coid_F;		//идентификаторы соединеня
	int timer_W, timer_F;	//идентификаторы таймера
	int print_w, print_f;	//идентификаторы вывода по W F
	if (prog_start == 1)		//если 1-й запуск, то задаем начальные значения
			{
		w_cnt = f_cnt = 32;
	};
	xx = x_old;		//сохранение предыдущих знач-й
	yy = y_old;
	zz = z_old;
	w_cnt = w_old;
	f_cnt = f_old;
	chid_WF = ChannelCreate(0);		//создаем канал
	msg.buf[0] = chid_WF;
	printw("Channel WF namber: <%d>\n", msg.buf);
	pid_t PID;
	PID = getpid();		//получить программный ID процесса
	coid_W = ConnectAttach(0, PID, chid_WF, 0, 0);		//создаем соединение
	coid_F = ConnectAttach(0, PID, chid_WF, 0, 0);
	struct sigevent event_W;		//объявление структуры события
	struct sigevent event_F;
	//заполнения структур для таймеров (SIGEV_PULSE_INIT)
	SIGEV_PULSE_INIT(&event_W, coid_W, SIGEV_PULSE_PRIO_INHERIT, TIMER_W_COUNT,
			NULL);
	SIGEV_PULSE_INIT(&event_F, coid_F, SIGEV_PULSE_PRIO_INHERIT, TIMER_F_COUNT,
			NULL);
	timer_W = TimerCreate(CLOCK_REALTIME, &event_W);		//создание таймера
	timer_F = TimerCreate(CLOCK_REALTIME, &event_F);
	msg.type = 7;
	printw("Init W...\n");
	status = MsgSend(coid_roby, &msg, sizeof(msg), NULL, 0);
	msg.type = 8;
	printw("Init F...\n");
	status = MsgSend(coid_roby, &msg, sizeof(msg), NULL, 0);
	while (1) {
		rcvd = MsgReceivePulse(chid_WF, &Pulse, sizeof(Pulse), NULL);
		//прием импульсного сообщения
		//printw("STAT: <%d>\t",rcvd);
		//printw("Code: <%d>\t",Pulse.code);
		//printw("Value: <%d>\n",Pulse.value.sival_int);
		switch (Pulse.code)		//проверяем код импульса
		{
		case 1:		//если комада запустить таймер по W
			itime.nsec = 5000000;		//время 1-го срабатывания таймера
			itime.interval_nsec = 160000000;		//интервал таймера
			TimerSettime(timer_W, NULL, &itime, NULL);		//запуск таймера
			break;

		case 33:		//"бежим" по W
			if (stop_w == 1)		//если поступила команда об остановке
					{			 //в нужной координате
				itime.nsec = 0;			 //останавливаем таймер
				itime.interval_nsec = 0;
				TimerSettime(timer_W, NULL, &itime, NULL);
				printw("Code: <%d> Value: <%d>\n", Pulse.code,
						Pulse.value.sival_int);
				stop_w = 0;			 //обнуляем статус остановки по W
				print_w = 1;			 //статус вывода по W=1 чтобы повторно
			}			  //не вывести одну и ту же строку
			else			  //если команда остановки отсутствует
			{	//считаем координаты по W
				w_cnt = w_cnt + state_W;
			}
			;
			break;

		case 8:	//если дошли по W до конца
			itime.nsec = 0;
			itime.interval_nsec = 0;
			TimerSettime(timer_W, NULL, &itime, NULL);
			printw("------------------------\n");
			printw("Code: <%d> Value: <%d>\n", Pulse.code,
					Pulse.value.sival_int);
			break;

		case 16:	//если дошли по W до начала
			itime.nsec = 0;
			itime.interval_nsec = 0;
			TimerSettime(timer_W, NULL, &itime, NULL);
			printw("------------------------\n");
			printw("Code: <%d> Value: <%d>\n", Pulse.code,
					Pulse.value.sival_int);
			break;

		case 2:	//если команда запустить таймер по F
			itime.nsec = 5000000;
			itime.interval_nsec = 170000000;
			TimerSettime(timer_F, NULL, &itime, NULL);
			break;

		case 65:	//"бежим" по F
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

		case 32:	//если до конца по F
			itime.nsec = 0;
			itime.interval_nsec = 0;
			TimerSettime(timer_F, NULL, &itime, NULL);
			printw("------------------------\n");
			printw("Code: <%d> Value: <%d>\n", Pulse.code,
					Pulse.value.sival_int);
			break;

		case 64:	//если дошли до начала по F
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
		//выводим пока не сработает статус вывода
		//(избегаем повторного вывода одной и той же строки)
		if ((print_w != 1) && (print_f != 1)) {
			printw("X=%d Y=%d Z=%d W=%d F=%d S=0 D=0\n", xx, yy, zz, w_cnt,
					f_cnt);
		};
		//обнуляем статусы вывода
		if (print_w == 1) {
			print_w = 0;
		};
		if (print_f == 1) {
			print_f = 0;
		};
	}
	ConnectDetach(coid_W);	//уничтожить соединение
	ConnectDetach(coid_F);
	ChannelDestroy(chid_WF);	//уничтожить созданный канал
	pthread_exit(NULL);	//уничтожить созданный поток
}
/* Ф-ция ввода/вывода нужных координат */
void *PrintCoor() {
	int active, active_msg;	//статус активной координаты, вывода
	int s_value, d_value, s_old, d_old;	//значения S D
	int blok_sd;		//статус блокировки S D
	char command[10];	//команда, отсылаемая роботу
	unsigned char rmsg;	//возвращаемое сообщение
	pthread_t thread11, thread22;	//идентификаторы потоков
	active = stop_done = 0;	//обнуляем активную координату, статус выполнения
	active_msg = 0;		//сообщение еще не отсылалось
	main_pulse_priority = 10;		//приоритет импульса
	if (prog_start == 1)		//если 1-й запуск программы
			{
		x_old = y_old = z_old = 512;
		w_old = f_old = 32;
		//s_old=d_old=0;
	};
	xx_old = x_old;		//сохраняем предыдущие значения
	yy_old = y_old;
	zz_old = z_old;
	ww_old = w_old;
	ff_old = f_old;
	//создаем новый поток для соединения с DisplayXYZ и DisplayWF
	pthread_create(&thread11, NULL, &DisplayXYZ, NULL);
	sleep(1);		//ждем 1 сек. чтобы соединился
	pthread_create(&thread22, NULL, &DisplayWF, NULL);
	sleep(1);
	pid_t PID;
	PID = getpid();		//получаем прогаммный ID процесса
	coid_WF = ConnectAttach(0, PID, chid_WF, 0, 0);		//содаем соединение
	//устанавливаем параметр запрета для завершения потока
	//PTHREAD_CANCEL_DISABLE - команда pthread_cancel будет проигнорирована
	//PTHREAD_CANCEL_ENABLE - команда будет выполнена
	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
	while (stop_done < 7)	//пока не проверит все введенные значения
	{
		if (active == 0)	//если робот еще не выполняет команды
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

		if (x_value != x_old)	//если новое и старое значения различны
				{
			//если активен Х и "бежим" вперед
			if (((active == 0) && (x_value > x_old))
					|| ((active == 1) && (x_value > x_old))) {//выполнили проверку на 1/7 (stop_done=1)
				active = stop_done = 1;	//активным становится Х
				if (active_msg == 0)	//если еще не отсылали команду
						{
					strcpy(command, "A04");
					msg.type = 0;
					sscanf(command + 1, "%X", &msg.buf);
					status = MsgSend(coid_roby, &msg, sizeof(msg), &rmsg,
							sizeof(rmsg));
					active_msg = 1;	//команда отослана
				};
				if (xx == x_value)	//если дошли до нужно координаты
						{			   //останавливаем робота по Х
					active = 0;			   //активного нет
					active_msg = 0;	//обнуляем для посыла сообщения в др. координатах
					strcpy(command, "A0");
					msg.type = 0;
					sscanf(command + 1, "%X", &msg.buf);
					status = MsgSend(coid_roby, &msg, sizeof(msg), &rmsg,
							sizeof(rmsg));
					x_old = x_value;			//запоминаем предыдущее значение
				};
			};

			//если активен Х и "бежим" назад
			if (((active == 0) && (x_value < x_old))
					|| ((active == 1) && (x_value < x_old))) {//выполнили проверку на 1/7 (stop_done=1)
				active = stop_done = 1;	//активным становится Х
				if (active_msg == 0)	//если еще не отсылали команду
						{
					strcpy(command, "A08");
					msg.type = 0;
					sscanf(command + 1, "%X", &msg.buf);
					status = MsgSend(coid_roby, &msg, sizeof(msg), &rmsg,
							sizeof(rmsg));
					active_msg = 1;	//команда отослана
				};
				if (xx == x_value)	//если дошли до нужной координаты
						{			   //останавливаем робота по Х
					active = 0;			   //активного нет
					active_msg = 0;	//обнуляем для посыла сообщения в др. координатах
					strcpy(command, "A0");
					msg.type = 0;
					sscanf(command + 1, "%X", &msg.buf);
					status = MsgSend(coid_roby, &msg, sizeof(msg), &rmsg,
							sizeof(rmsg));
					x_old = x_value;			//запоминаем предыдущее значение
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

		if (w_value != w_old)			 //если новое и старое значения различны
				{	//если активен W и "бежим" вперед
			if (((active == 0) && (w_value > w_old))
					|| ((active == 4) && (w_value > w_old))) {//выполнили проверку на 4/7 (stop_done=4)
				active = stop_done = 4;	//активен W
				if (active_msg == 0)	//если еще не отсылали сообщение
						{
					strcpy(command, "C10");
					msg.type = 1;
					sscanf(command + 1, "%X", &msg.buf);
					state_W = 1;	//т.к. "бежим" вперед
					//отсылаем импульсное сообщение о старте таймера по W
					status_WF = MsgSendPulse(coid_WF, main_pulse_priority,
							TIMER_W_SET, NULL);
					status = MsgSend(coid_roby, &msg, sizeof(msg), &rmsg,
							sizeof(rmsg));
					active_msg = 1;						//сообщение отослано
				};
				if (w_cnt == w_value)		//если достигли нужной координаты
						{				  //останавливаем робота по W
					stop_w = 1;				  //статус остановки по W
					active = 0;				  //активного нет
					active_msg = 0;				 //обнуляем для других координат
					strcpy(command, "C0");
					msg.type = 1;
					sscanf(command + 1, "%X", &msg.buf);
					status = MsgSend(coid_roby, &msg, sizeof(msg), &rmsg,
							sizeof(rmsg));
					w_old = w_value;				  //запоминаем предыдущее
				};
			};

			//если активный W и "бежим" назад
			if (((active == 0) && (w_value < w_old))
					|| ((active == 4) && (w_value < w_old))) {//выполнили проверку на 4/7 (stop_done=4)
				active = stop_done = 4;	//активен W
				if (active_msg == 0)	//если не отсылали сообщение
						{
					strcpy(command, "C20");
					msg.type = 1;
					sscanf(command + 1, "%X", &msg.buf);
					state_W = -1;	//т.к. "бежим" назад
					//отсылаем импульсное сообщение о старте таймера по W
					status_WF = MsgSendPulse(coid_WF, main_pulse_priority,
							TIMER_W_SET, NULL);
					status = MsgSend(coid_roby, &msg, sizeof(msg), &rmsg,
							sizeof(rmsg));
					active_msg = 1;						//сообщение отослано
				};
				if (w_cnt == w_value)			//если нашли нужную координату
						{				  //тормозим робота по W
					stop_w = 1;				  //статус остановки по W
					active = 0;				  //активного нет
					active_msg = 0;				 //обнуляем для других координат
					strcpy(command, "C0");
					msg.type = 1;
					sscanf(command + 1, "%X", &msg.buf);
					status = MsgSend(coid_roby, &msg, sizeof(msg), &rmsg,
							sizeof(rmsg));
					w_old = w_value;			//запоминаем предыдущее значение
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

		blok_sd = 0;		//блокировка (выполняется за раз только процесс 1)
		//if выполнится если уже проверены X Y Z W F
		if (((active == 0) && (s_value == 1)) && (blok_sd == 0)) {//выполнили проверку на 6/7 (stop_done=6)
			stop_done = 6;
			strcpy(command, "A02");
			msg.type = 0;
			sscanf(command + 1, "%X", &msg.buf);
			status = MsgSend(coid_roby, &msg, sizeof(msg), &rmsg, sizeof(rmsg));
			s_old = s_value;	//запоминаем предыдущее значение
			blok_sd = 1;		//блокируем D на время
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
		//если проверили все значения
		if (stop_done == 7) {	//выводим их и разрешаем остановить поток
			printw("--------\n");
			printw("X=%d Y=%d Z=%d W=%d F=%d S=%d D=%d\n", xx, yy, zz, w_cnt,
					f_cnt, s_value, d_value);
			pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
		};
	};
	ConnectDetach(coid_WF);			//уничтожить соединение
	pthread_cancel(thread11);		//остановить поток
	pthread_join(thread11, NULL);	//ждем остановки потока
	printw("Done THREAD_XYZ.\n");
	pthread_cancel(thread22);
	pthread_join(thread22, NULL);
	printw("Done THREAD_WF.\n");
	pthread_exit(NULL);	//уничтожить созданный поток
	return EXIT_SUCCESS;
}

/* Ф-ция вывода состояния датчиков */
void *PrintSensors() {
	int x_start, y_start, z_start;	//значения датчиков
	int w_start, w_end;
	int f_start, f_end;
	x_start = y_start = z_start = 0;	//обнулим при старте
	w_start = f_start = 0;
	w_end = f_end = 0;
	stop_sensor = 0;//статус выполнения вывода датчиков (для остановки ф-ции)
	//printw("X=%d Y=%d Z=%d W=%d F=%d\n",x_value,y_value,z_value,w_value,f_value);
	//printw("OLD X=%d Y=%d Z=%d W=%d F=%d\n",xx_old,yy_old,zz_old,ww_old,ff_old);
	if (x_value != xx_old)	//если перемещались по Х
			{
		x_start = 1;	//датчик вкл.
	};
	if (y_value != yy_old) {
		y_start = 1;
	};
	if (z_value != zz_old) {
		z_start = 1;
	};
	if ((w_value != ww_old) && (w_value > ww_old))	//если "бежали" вперед по W
			{
		w_end = 1;	//конечный датчик вкл.
	};
	if ((w_value != ww_old) && (w_value < ww_old))	//если назад
			{
		w_start = 1;	//начальный вкл.
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
	stop_sensor = 1;	//все выведено - остановить ф-цию
	pthread_exit(NULL);	//уничтожить созданный поток
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

	chid_TWF = ChannelCreate(0);                // Создание канала для таймеров.

	ConAtt1 = ConnectAttach(0, 0, chid_TWF, 0, 0); // Создание канального соединения с 1-ым таймером.
	ConAtt2 = ConnectAttach(0, 0, chid_TWF, 0, 0); // Создание канального соединения со 2-ым таймером.
	ConAtt3 = ConnectAttach(0, 0, chid_main, 0, 0);
	ConDisplayXYZWF = ConnectAttach(0, main_coid, chid_XYZWF, 0, 0); // Создание канального соединения с потоком DisplayXYZWF.

	SIGEV_PULSE_INIT(&event1, ConAtt1, 1, 0, TIMER_W_COUNT); // Заполнение структуры sigevent для таймера координаты W.
	SIGEV_PULSE_INIT(&event2, ConAtt2, 1, 0, TIMER_F_COUNT); // Заполнение структуры sigevent для таймера координаты F.
	timer_W = TimerCreate(CLOCK_SOFTTIME, &event1); // Создание виртуального таймера для координаты W.
	timer_F = TimerCreate(CLOCK_SOFTTIME, &event2); // Создание виртуального таймера для координаты F.

	while (1) {
		print = 0;
		MsgReceivePulse(chid_TWF, &pl, sizeof(pl), NULL); // Прием импульсного сообщения.

		switch (pl.value[2]) {
		case TIMER_W_SET:        // Если нужно запустить таймер по координате W.
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

			MsgSend(coid_roby, &msg, sizeof(msg), NULL, 0);   // Выдать команду.
			break;

		case TIMER_F_SET:
			// Если нужно запустить таймер по координате F.
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
			// Если нужно остановить таймер по координате W.
		case TIMER_F_UNSET:
			itime_W.nsec = 0;
			itime_W.interval_nsec = 0;
			TimerSettime(timer_F, NULL, &itime_W, NULL);
			break;

		case TIMER_W_COUNT:               // Увеличение/уменьшение координаты W.
			if (state_W == 1) {
				w_cnt++;
			} else {
				w_cnt--;
			}                                 //if
			if (w_cnt == number) {          // Не достигнем требуемого значения.
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

		case TIMER_F_COUNT:               // Увеличение/уменьшение координаты F.
			if (state_F == 1) {
				f_cnt++;
			} else {
				f_cnt--;
			}                                 //if
			if (f_cnt == number) {          // Не достигнем требуемого значения.
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
			MsgSendPulse(ConDisplayXYZWF, 1, 0, DEFAULT_PULSE); // Сообщить потоку DisplayXYZWF об изменении координат WF.
		}    //if
//      sched_yield();

	}    //while
}    //TimerWF

void *SensorWF() {
	int i;
	struct PULSE pl;
	struct MESSAGE msg;

	chid_SWF = ChannelCreate(0); // Создание канала для обработки датчиков конечных положений.

	for (i = 7; i < 9; i++) {            //Иницивлизация датчиков положения F,W
		msg.type = i;
		msg.buf[0] = chid_SWF;
		MsgSend(coid_roby, &msg, sizeof(msg), NULL, 0);
	};

	while (1) {
		MsgReceivePulse(chid_SWF, &pl, sizeof(pl), NULL); // Прием импульсного сообщения.
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
	unsigned char PA = 0, PC = 0; //Эти переменные хранят значения регистров PA и PC
	//робота Roby.
	int ch; //Символ, введённый пользователем с клавиатуры.
	char Help[] =
			"Unknown command. Please, use next keys: <F1>-<F12>, <Enter>, <+/=> or <Esc> for exit."; //Справка.
	char command; //Тип команды.

	int status;
	struct MESSAGE msg; //Посылаемое сообщение.
	unsigned char rmsg; //Буфер ответного сообщения.
	//char command[10]; //Буфер для команд оператора.

	int coid; //Идентификатор соединения, который использует эмулятор робота Roby.

	coid_roby = coid = name_open("apu/roby", NULL); //Определить coid (идентификатор соединения)
													//по имени "apu/roby".
	printw("apu/roby has coid=%d \n", coid);
	pthread_t thread33;	//идентификатор потока
	pthread_create(&thread33, NULL, &PrintCoor, NULL);

	//соединение с каналом chidWF (для запуска таймеров).
	sleep(5); //Приостановить поток main, чтобы позволить потокам DisplayXYZWF
	//подготовиться к работе (например, инициализировать датчики движения по X, Y и Z
	//и датчики крайних положений по W и F эмулятора робота roby).
//
//	/*****Syntax for ConnectAttach()*****************
//	 * int ConnectAttach(int nd, //Node Descriptor - дескриптор узла в сети (nd=0, если
//	 *				//локальное соединение);
//	 *		pid_t pid, //PID (Process ID);
//	 *		int chid, //CHID (Channel ID);
//	 *		unsigned index,
//	 *		int flags); //Флаги.
//	 ************************************************/
	printw("Pult1_var2: main thread: please, press any key.\n");
	do { //Бесконечный цикл.
		sleep(1); //Передать управление потоку DisplayXYZ или
		//потоку DisplayWF (на некоторое время).

		//Интерфейс с пользователем.
		ch = getch(); //getchar();//getch(); //Считать символ, введённый с клавиатуры.
		switch (ch) { //Анализ ASCII-кода нажатой клавиши.
		case 27: //Нажата клавиша <Esc>.
			printw("\n<Esc> - exit..."); //Завершение программы.
			getch(); //getch();
			command = 'E'; //Тип команды.
			return 0;
		case 13: //Нажата клавиша <Enter>.
			printw("<Enter> - move to start position on all coordinates"
					"\nand clear position values.");
			//Двигаться в начальное положение по всем координатам
			//и сбросить значения датчиков положений.
			//command=...; //Тип команды.
			PA = 0;
			PC = 0; //Для этого сбросить в 0 переменные PA и PC.
			command = 'C'; //Подать команду записи в регистр PC роботу Roby.
			break;
		case '+':
		case '=': //Нажата клавиша <+/=>.
			//Переключить режим отображения состояния датчиков.
			DisplayState ^= 0x01; //Поразрядное исключающее ИЛИ (XOR)
			//для переменной DisplayState сбрасывает её в 0
			//либо устанавливает в 1.
			printw("\n<+/=> - display state = %d.", DisplayState);
			command = 'W';
			break;
		case 0: //Нажата управляющая или функциональная клавиша.
			switch (ch = getch()) { //Это требует дополнительного анализа.
			case 59: //Нажата клавиша <F1>.
				//Начать/остановить движение по X вперёд.
				printw("\n<F1> - toggle moving on X forward.");
				PA ^= A_X_FORWARD; //Переключить бит A_X_FORWARD в регистре PA.
				command = 'A'; //Тип команды.
				break;
			case 60: //Нажата клавиша <F2>.
				//Начать/остановить движение по X назад.
				printw("\n<F2> - toggle moving on X backward.");
				PA ^= A_X_BACK; //Переключить бит A_X_BACK в регистре PA.
				command = 'A'; //Тип команды.
				break;
			case 61: //Нажата клавиша <F3>.
				//Начать/остановить движение по Y вперёд.
				printw("\n<F3> - toggle moving on Y forward.");
				PA ^= A_Y_FORWARD; //Переключить бит A_Y_FORWARD в регистре PA.
				command = 'A'; //Тип команды.
				break;
			case 62: //Нажата клавиша <F4>.
				//Начать/остановить движение по Y назад.
				printw("\n<F4> - toggle moving on Y backward.");
				PA ^= A_Y_BACK; //Переключить бит A_Y_BACK в регистре PA.
				command = 'A'; //Тип команды.
				break;
			case 63: //Нажата клавиша <F5>.
				//Начать/остановить движение по Z вперёд.
				printw("\n<F5> - toggle moving on Z forward.");
				PA ^= A_Z_FORWARD; //Переключить бит A_Z_FORWARD в регистре PA.
				command = 'A'; //Тип команды.
				break;
			case 64: //Нажата клавиша <F6>.
				//Начать/остановить движение по Z назад.
				printw("\n<F6> - toggle moving on Z backward.");
				PA ^= A_Z_BACK; //Переключить бит A_Z_BACK в регистре PA.
				command = 'A'; //Тип команды.
				break;
			case 65: //Нажата клавиша <F7>.
				//Начать/остановить движение по F вперёд.
				printw("\n<F7> - toggle rotating on F forward.");
				PC ^= C_F_FORWARD; //Переключить бит C_F_FORWARD в регистре PC.
				command = 'C'; //Тип команды.
				ActiveF = 1; //F - активная координата.
				if (stateF == 1) //Если Roby движется вперёд по F,
					stateF = 0; //то стоп по F.
				else
					//Иначе
					stateF = 1; //запустить движение вперёд по F.
				break;
			case 66: //Нажата клавиша <F8>.
				//Начать/остановить движение по F назад.
				printw("\n<F8> - toggle rotating on F backward.");
				PC ^= C_F_BACK; //Переключить бит C_F_BACK в регистре PC.
				command = 'C'; //Тип команды.
				ActiveF = 1; //F - активная координата.
				if (stateF == -1) //Если Roby движется назад по F,
					stateF = 0; //то стоп по F.
				else
					//Иначе
					stateF = -1; //запустить движение назад по F.
				break;
			case 67: //Нажата клавиша <F9>.
				//Начать/остановить движение по W вперёд.
				printw("\n<F9> - toggle rotating head on W forward.");
				PC ^= C_W_FORWARD; //Переключить бит C_W_FORWARD в регистре PC.
				command = 'C'; //Тип команды.
				ActiveW = 1; //W - активная координата.
				if (stateW == 1) //Если Roby двигался вперёд по W,
					stateW = 0; //то остановить его по W.
				else
					//Иначе
					stateW = 1; //запустить движение вперёд по W.
				break;
			case 68: //Нажата клавиша <F10>.
				//Начать/остановить движение по W назад.
				printw("\n<F10> - toggle rotating head on W backward.");
				PC ^= C_W_BACK; //Переключить бит C_W_BACK в регистре PC.
				command = 'C'; //Тип команды.
				ActiveW = 1; //W - активная координата.
				if (stateW == -1) //Если Roby двигался назад по W,
					stateF = 0; //то остановить его по W.
				else
					//Иначе
					stateF = -1; //запустить движение назад по W.
				break;
			case 133: //Нажата клавиша <F11>.
				//Включить/выключить схват S.
				printw("\n<F11> - toggle S.");
				PA ^= A_S; //Переключить бит A_S в регистре PA.
				command = 'A'; //Тип команды.
				break;
			case 134: //Нажата клавиша <F12>.
				//Включить/выключить дрель D.
				printw("\n<F12> - toggle drill work.");
				PA ^= A_D; //Переключить бит A_D в регистре PA.
				command = 'A'; //Тип команды.
				break;
			} //End of switch().
			break;
		default: //Нажата другая клавиша.
			printw("\n%s", Help); //puts(Help);
			command = 'U'; //Неизвестный тип команды.
		} //End of switch().
		if (DisplayState == 1) { //Если включен режим отображения состояния датчиков, то
			//command='b'; //Команда чтения из порта PB роботу Roby.
			command = 'c'; //Команда чтения из порта PC роботу Roby.
		} //End of if().
		  //.....Анализ типа команды command.
		switch (command) { //Анализировать тип команды, заданный символом command.
		case 'A':       //Write port A.
			msg.type = 0; //Тип сообщения.
			msg.buf[0] = PA; //Записать в буфер сообщения msg.buf значение переменной PA.
			//sscanf(command+1,"%X", &msg.buf); //Преобразовать строку (command+1)
			//из 16-ичного вида и сохранить результат в msg.buf.
			printw("Write port A.");
			status = MsgSend(coid, &msg, sizeof(msg), &rmsg, sizeof(rmsg));
			//int MsgSend( int coid, //Послать сообщение, где coid - идентификатор соединения;
			//             const void* smsg, //smsg - указатель на посылаемое сообщение;
			//             int sbytes, //sbytes - размер (в байтах) посылаемого сообщения;
			//             void* rmsg, //rmsg - указатель на буфер принимаемого ответа от roby;
			//             int rbytes ); //rbytes - размер (в байтах) буфера для принимаемого ответа.
			break;

		case 'C':       //Write port C.
			msg.type = 1; //Тип сообщения.
			msg.buf[0] = PC; //Записать в буфер сообщения msg.buf значение переменной PC.
			//sscanf(command+1,"%X", &msg.buf); //Считать 16-ичную строку из (command+1)
			//в буфер msg.buf.
			printw("Write port C.");
			//.....Анализ входной комманды оператора:
			//определить координату движения (W или F); (stateW или stateF уже
			//установлены) послать импульсные сообщения для запуска таймеров
			//потоку DisplayWF, используя MsgSendPulse().

			/*****Syntax for MsgSendPulse()************************
			 * MsgSendPulse(int coid, //соединение;
			 *		int priority, //приоритет импульса;
			 *		char code, //код (тип) импульса;
			 *		int value); //значение.
			 ******************************************************/

			if (ActiveW) //Если стала активной координата W, то
				MsgSendPulse(coid_WF, main_pulse_priority, TIMER_W_SET, NULL); //запустить таймер по W.
			if (ActiveF) //Если стала активной координата F, то
				MsgSendPulse(coid_WF, main_pulse_priority, TIMER_F_SET, NULL); //запустить таймер по F.

			status = MsgSend(coid, &msg, sizeof(msg), &rmsg, sizeof(rmsg)); //Послать
			//сообщение msg к roby.
			break;

		case 'c':       //Read port C.
			msg.type = 2; //Тип сообщения.
			//Буфер сообщения msg.buf при чтении порта C заполнять не требуется.
			printw("Read port C.");
			status = MsgSend(coid, &msg, sizeof(msg), &rmsg, sizeof(rmsg)); //Послать
			//сообщение msg к roby и принять ответ rmsg от roby.
			printw("Port C: 0x%X", rmsg); //Вывод на экран (в поток stdout) ответ rmsg
			//в 16-ичном виде.
			break;

		case 'b':       //Read port B.
			msg.type = 3; //Тип сообщения.
			//Буфер сообщения msg.buf при чтении порта B заполнять не требуется.
			//sscanf(command+1,"%X", &msg.buf); //*(command+1) => msg.buf
			printw("Read port B.");
			status = MsgSend(coid, &msg, sizeof(msg), &rmsg, sizeof(rmsg)); //Послать
			//сообщение msg к roby и принять ответ rmsg от roby.
			printw("Port B: 0x%X", rmsg); //Вывод на экран (в поток stdout) ответ rmsg
			//в 16-ичном виде.
			break;
		case 'W': //Смена вывода, нет действий для робота
			break;
		case 'E': //Выход из программы.
			//endwin();
			return 0;       //Return from main().

		default:	//Другой символ.
			printw("Unknown command\n");
			break;
		}
	} while (1);
	printw("Stop Work \n");
	getch();
	endwin();
	return EXIT_SUCCESS;
}
