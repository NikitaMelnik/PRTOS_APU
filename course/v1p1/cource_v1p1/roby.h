//���� "roby.h".

//���� ॣ���� A:
#define  A_D            0x01 //D: �ࠢ����� �५��;
#define  A_S	        0x02 //S: �ࠢ����� �墠⮬;
#define  A_X_FORWARD 	0x04 //+X: �������� �� X �����;
#define  A_X_BACK	0x08 //-X: �������� �� X �����;
#define  A_Z_BACK    	0x10 //-Z: �������� �� Z �����;
#define  A_Z_FORWARD 	0x20 //+Z: �������� �� Z �����;
#define  A_Y_BACK    	0x40 //-Y: �������� �� Y �����;
#define  A_Y_FORWARD	0x80 //+Y: �������� �� Y �����.

//���� ॣ���� B:
#define  B_X            0x01 //Xp (Xpulse): ������� ���稪 �������� �� X;
#define  B_Y            0x02 //Yp (Ypulse): ������� ���稪 �������� �� Y;
#define  B_Z            0x04 //Zp (Zpulse): ������� ���稪 �������� �� Z;
#define  B_W_END	0x08 //WE (WEnd): ���稪 ����筮�� ��������� �� W (��� �������);
#define  B_W_BEGIN	0x10 //WB (WBegin): ���稪 ��砫쭮�� ��������� �� W (��� �������);
#define  B_Z_BEGIN	0x20 //ZN: ���稪 ��砫쭮�� ��������� �� Z;
#define  B_Y_BEGIN	0x40 //YN: ���稪 ��砫쭮�� ��������� �� Y;
#define  B_X_BEGIN	0x80 //XN: ���稪 ��砫쭮�� ��������� �� X.

//���� ॣ���� C: (�� �⮬ ���� PC0 � PC1 �� �ᯮ�������)
#define  C_F_END        0x04 //FE (FEnd): ���稪 ����筮�� ��������� �� F (��� �᭮�����);
#define  C_F_BEGIN      0x08 //FB (FBegin): ���稪 ��砫쭮�� ��������� �� F (��� �᭮�����);
#define  C_W_FORWARD 	0x10 //+W: �������� �� W ����� (��� �������);
#define  C_W_BACK    	0x20 //-W: �������� �� W ����� (��� �������);
#define  C_F_FORWARD 	0x40 //+F: �������� �� F ����� (��� �᭮�����);
#define  C_F_BACK    	0x80 //-F: �������� �� F ����� (��� �᭮�����).

//.....��� ��⮪� DisplayXYZ.....
//���� ��������, ��।���� ���祭�� ���न��� X, Y � Z �� ஡�� Roby:
#define X_VALUE		-1 //��� ����� ᨣ����� �ᯮ������� ����⥫�� ���祭��,
#define Y_VALUE		-2 //⠪ ��� ������⥫�� ���祭�� ����� ᨣ�����
#define Z_VALUE		-4 //�ᯮ���� ��⥬�.

//.....��� ��⮪� DisplayWF.....
//���� (���� code) ���������� ᮮ�饭�� �� ���稪�� �ࠩ��� ���������
	//�� ���न��⠬ W � F:
#define W_END		0x08
#define W_BEGIN		0x10
#define F_END		0x20
#define F_BEGIN		0x40

//���� ���������� ᮮ�饭�� ��� ����᪠ ⠩��஢ � ��⮪� DisplayWF:
#define TIMER_W_SET	1
#define TIMER_F_SET	2

//���� �����ᮢ ⠩��� ��� ���������� �������� (㢥��������) sigevent
	//� ��⮪� DisplayWF:
#define TIMER_W_COUNT	33 //⠩��� ���न���� W;
#define TIMER_F_COUNT	65 //⠩��� ���न���� F.

