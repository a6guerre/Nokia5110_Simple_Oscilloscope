/*
 * UART_Driver.c
 *
 *  Created on: Feb 22, 2020
 *      Author: Max
 */
#include "tm4c123gh6pm.h"
//#include "utils/uartstdio.h"
#include <stdint.h>

void UART_Init(void){
  SYSCTL_RCGC1_R |= SYSCTL_RCGC1_UART0; // activate UART0
  SYSCTL_RCGC2_R |= SYSCTL_RCGC2_GPIOA; // activate port A
  UART0_CTL_R &= ~UART_CTL_UARTEN;      // disable UART
  double ibrd = 80000000/(16*115200);
  int int_ibrd_r = (int)ibrd;
  UART0_IBRD_R = int_ibrd_r;                    // IBRD = int(12,500,000 / (16 * 115,200)) = int(6.7816)
  UART0_FBRD_R = 50;                            // FBRD = int(0.7816 * 64 + 0.5) = 8
                                                // 8 bit word length (no parity bits, one stop bit, FIFOs)
  UART0_LCRH_R = (UART_LCRH_WLEN_8|UART_LCRH_FEN);
  UART0_CTL_R |= UART_CTL_UARTEN;       // enable UART
  GPIO_PORTA_AFSEL_R |= 0x03;           // enable alt funct on PA1-0
  GPIO_PORTA_DEN_R |= 0x03;             // enable digital I/O on PA1-0
}

void UART1_Init(void){
  SYSCTL_RCGC1_R |= SYSCTL_RCGC1_UART1; // activate UART0

  UART1_CTL_R &= ~UART_CTL_UARTEN;      // disable UART
  UART1_IBRD_R = 6;                    // IBRD = int(12,500,000 / (16 * 115,200)) = int(6.7816)
  UART1_FBRD_R = 50;                     // FBRD = int(0.7816 * 64 + 0.5) = 8
                                        // 8 bit word length (no parity bits, one stop bit, FIFOs)
  UART1_LCRH_R = (UART_LCRH_WLEN_8|UART_LCRH_FEN);
  UART1_CTL_R |= UART_CTL_UARTEN;       // enable UART
  GPIO_PORTB_AFSEL_R |= 0x03;           // enable alt funct on PA1-0
  GPIO_PORTB_DEN_R |= 0x03;             // enable digital I/O on PA1-0
}

void UART1_OutChar(unsigned char data){
  while((UART1_FR_R&UART_FR_TXFF) != 0);
  UART1_DR_R = data;
}

void UART1_OutUDec(unsigned long n){
// This function uses recursion to convert decimal number
//   of unspecified length as an ASCII string
  if(n >= 10){
    UART1_OutUDec(n/10);
    n = n%10;
  }
  UART1_OutChar(n+'0'); /* n is between 0 and 9 */
}


void UART_OutChar(unsigned char data){
  while((UART0_FR_R&UART_FR_TXFF) != 0);
  UART0_DR_R = data;
}

void UART_OutUDec(unsigned long n){
// This function uses recursion to convert decimal number
//   of unspecified length as an ASCII string
  if(n >= 10){
    UART_OutUDec(n/10);
    n = n%10;
  }
  UART_OutChar(n+'0'); /* n is between 0 and 9 */
}

void UART_OutFloat(float out_float)
{

  unsigned char decimal_part = (out_float - (unsigned char)out_float)*100;
  UART_OutUDec((uint8_t)out_float);
  UART_OutChar('.');
  UART_OutUDec(decimal_part);
  UART_OutChar('\n');
}
