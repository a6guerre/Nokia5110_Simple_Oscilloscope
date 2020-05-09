/*
 * Nokia5110.h
 *
 *  Created on: May 7, 2020
 *      Author: a6gue
 */

#ifndef NOKIA5110_H_
#define NOKIA5110_H_


#define  PD 2
#define  V  1
#define  H  0

#define RESET_PIN 0x80

typedef enum
{
  COMMAND = 0,
  DATA = 1
}xmit_type_t;

void Nokia5110_SetCursor(unsigned char newX, volatile unsigned char newY);

void LCD_Write(xmit_type_t mode, char data);

void SetState_Reset();

void Clear_Display();

void Nokia5110_Config();

void InitializeNokia5110(void);

#endif /* NOKIA5110_H_ */
