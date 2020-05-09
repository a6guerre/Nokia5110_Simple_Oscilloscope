/*
 * UART_Driver.h
 *
 *  Created on: Feb 22, 2020
 *      Author: Max
 */

#ifndef DRIVERS_HC_S04_DISTANCE_SENSOR_UART_DRIVER_H_
#define DRIVERS_HC_S04_DISTANCE_SENSOR_UART_DRIVER_H_

void UART_Init(void);
void UART1_Init(void);
void UART1_OutChar(unsigned char data);
void UART1_OutUDec(unsigned long n);
void UART_OutChar(unsigned char data);
void UART_OutUDec(unsigned long n);
void UART_OutFloat(float out_float);

#endif /* DRIVERS_HC_S04_DISTANCE_SENSOR_UART_DRIVER_H_ */
