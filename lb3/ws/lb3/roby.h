/*
 * roby.h
 *
 *  Created on: 14.03.2016
 *      Author: Александр
 */

#ifndef ROBY_H_
#define ROBY_H_

//Биты регистра A
#define A_D 			0x01
#define A_S				0x02
#define A_X_FORWARD 	0x04
#define A_X_BACK 		0x08
#define A_Z_BACK 		0x10
#define A_Z_FORWARD 	0x20
#define A_Y_BACK 		0x40
#define A_Y_FORWARD 	0x80

//Биты регистра B
#define  B_X            0x01
#define  B_Y            0x02
#define  B_Z            0x04
#define  B_W_END       	0x08
#define  B_W_BEGIN     	0x10
#define  B_Z_BEGIN    	0x20
#define  B_Y_BEGIN     	0x40
#define  B_X_BEGIN     	0x80

//Биты регистра C
#define  C_F_END        0x04
#define  C_F_BEGIN      0x08
#define  C_W_FORWARD 	0x10
#define  C_W_BACK    	0x20
#define  C_F_FORWARD 	0x40
#define  C_F_BACK    	0x80

#endif /* ROBY_H_ */
