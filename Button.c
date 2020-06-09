/*
 * Button.c
 *
 *  Created on: May 12, 2020
 *      Author: a6gue
 */
#include <stdint.h>
#include "Button.h"
#include "tm4c123gh6pm.h"

uint8_t debounce = 1;

static void Config_Debounce_Timer(void)
{
    SYSCTL_RCGC1_R  |= SYSCTL_RCGC1_TIMER0;   // Enable Timer 0
    TIMER0_CTL_R    &= ~0x100;                 // Clear this while config
    TIMER0_CFG_R    |=  TIMER_CFG_16_BIT;     // Clear GPTMCFG bits (32-bit mode)
    TIMER0_TBMR_R   |=  0x02;                 // Periodic, count down.
    TIMER0_TBILR_R  |=  0xFFFF;               // The Register Value we will decrement from to 0
    TIMER0_TBPR_R   |=  0xFF;
    TIMER0_IMR_R    |=  0x100;                 // Set timer0B interrupt.
   // TIMER0_CTL_R    |=  0x100;                 // Enable Falling-Edge Trigger for Timer0B.
    NVIC_PRI5_R =   (NVIC_PRI5_R&0xFF0FFFFF)|0x00800000; // priority 4 (1000)
    NVIC_EN0_R |=   (1 << 20);                // Enable interrupt 20, according to NVIC Table
 //   TIMER0_CTL_R    =  0x100;                 // Enable Timer and start counting down.
}

void Button_Config(void)
{
    Config_Debounce_Timer();
    SYSCTL_RCGCGPIO_R   |=  0x10;  // Enable GPIO Port E
    GPIO_PORTE_DIR_R    &= ~0x02;  // Pin 1 is an input
    GPIO_PORTE_AFSEL_R  &= ~0x02;
    GPIO_PORTE_AMSEL_R  &= ~0x00;
    GPIO_PORTE_DEN_R    |=  0x02;
                                  // weak pull up resistor
    GPIO_PORTE_IS_R &= ~0x10;     // P is edge-sensitive
    GPIO_PORTE_IBE_R &= ~0x01;    // PE1 is not both edges
    GPIO_PORTE_IEV_R &= ~0x01;    // PE1 falling edge event
    GPIO_PORTE_ICR_R = 0x02;      // clear flag1
    GPIO_PORTE_IM_R |= 0x02;      // arm interrupt on PE1
    NVIC_PRI1_R = (NVIC_PRI1_R&0xFFFFFF0F)|0x00000060; // priority 3
    NVIC_EN0_R |= 0x00000010;      // (h) enable interrupt 4 (1 0000) in NVIC
}
