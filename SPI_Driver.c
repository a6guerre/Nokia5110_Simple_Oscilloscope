/*
 * SPI_Driver.c
 *
 *  Created on: May 6, 2020
 *      Author: a6gue
 */
#include <stdint.h>
#include "tm4c123gh6pm.h"
#include "SPI_Driver.h"

void Config_SPI()
{
    volatile unsigned long delay;
    SYSCTL_RCGC1_R |= SYSCTL_RCGC1_SSI0;  // activate SSI0
    SYSCTL_RCGC2_R |= SYSCTL_RCGC2_GPIOA; // activate port A
    delay = SYSCTL_RCGC2_R;               // allow time to finish activating
    GPIO_PORTA_DIR_R |= 0xC0;             // make PA6,7 out
    GPIO_PORTA_AFSEL_R |= 0x2C;           // enable alt funct on PA2,3,5
    GPIO_PORTA_AFSEL_R &= ~0xC0;          // disable alt funct on PA6,7
    GPIO_PORTA_DEN_R |= 0xEC;             // enable digital I/O on PA2,3,5,6,7
                                          // configure PA2,3,5 as SSI
    GPIO_PORTA_PCTL_R = (GPIO_PORTA_PCTL_R&0xFF0F00FF)+0x00202200;
                                          // configure PA6,7 as GPIO
    GPIO_PORTA_PCTL_R = (GPIO_PORTA_PCTL_R&0x00FFFFFF)+0x00000000;
    GPIO_PORTA_AMSEL_R &= ~0xEC;          // disable analog functionality on PA2,3,5,6,7
    SSI0_CR1_R &= ~SSI_CR1_SSE;           // disable SSI
    SSI0_CR1_R &= ~SSI_CR1_MS;            // master mode
                                          // configure for system clock/PLL baud clock source
    SSI0_CC_R = (SSI0_CC_R&~SSI_CC_CS_M)+SSI_CC_CS_SYSPLL;
                                          // clock divider for 3.077 MHz SSIClk (80 MHz PIOSC/26)
    SSI0_CPSR_R = (SSI0_CPSR_R&~SSI_CPSR_CPSDVSR_M)+26;
    SSI0_CR0_R &= ~(SSI_CR0_SCR_M |       // SCR = 0 (3.125 Mbps data rate)
                    SSI_CR0_SPH |         // SPH = 0
                    SSI_CR0_SPO);         // SPO = 0
                                          // FRF = Freescale format
    SSI0_CR0_R = (SSI0_CR0_R&~SSI_CR0_FRF_M)+SSI_CR0_FRF_MOTO;
                                          // DSS = 8-bit data
    SSI0_CR0_R = (SSI0_CR0_R&~SSI_CR0_DSS_M)+SSI_CR0_DSS_8;
    SSI0_CR1_R |= SSI_CR1_SSE;            // enable SSI

}
