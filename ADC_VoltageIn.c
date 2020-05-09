/*
 * main.c
 * Prototype of alpha version for ADC Voltage Measurements
 * Writing into memory at a periodic rate, drivem by a Timer 
 * peripheral and ADC Interrupt Handler.
 *
 */
#include "tm4c123gh6pm.h"
#include "UART_Driver.h"
#include <stdint.h>

float voltageIn;
/*
 * This function configures the GPIO Port and Pin that will 
 * be used for reading voltage input by the ADC Sampler.
 */
void GPIO_Input_Config(void)
{
  SYSCTL_RCGCGPIO_R   |=  0x10;  // Enable GPIO Port E
  GPIO_PORTE_DIR_R    &= ~0x10;  // Pin 4 is an input
  GPIO_PORTE_AFSEL_R  |=  0x10;
  GPIO_PORTE_AMSEL_R  |=  0x10;
  GPIO_PORTE_DEN_R    &= ~0x10;
}

/*
 * Configures the timer that will be driving the period of the 
 * ADC Samples
 */
void Trigger_Timer_Config(uint32_t load_val)
{
    SYSCTL_RCGC1_R  |= SYSCTL_RCGC1_TIMER0;   // Enable Timer 0
    TIMER0_CTL_R    &= ~0x01;       // Clear this while config
    TIMER0_CFG_R    |=  TIMER_CFG_16_BIT;       // Clear GPTMCFG bits (32-bit mode)
    TIMER0_TAMR_R   |=  0x02;       // Periodic, count down.
    TIMER0_TAILR_R  |=  load_val;    // The Register Value we will decrement from to 0
    TIMER0_IMR_R    &= ~0x01;       // No software timer interrupt.
                                    // (Time out will go to ADC Handler)
    TIMER0_CTL_R    |=  (0x01<<5);  // Enable ADC Trigger for Timer0A.
    TIMER0_CTL_R    |=  0x01;       // Enable Timer and start counting down.
}

void ADC_Init(void)
{
  SYSCTL_RCGCADC_R |= 0x01;
  Trigger_Timer_Config(6400);
  ADC0_SSPRI_R = 0x0123;        // Sequencer 3 is highest priority
  ADC0_ACTSS_R &= ~0x0008;      // disable sample sequencer 3
  ADC0_EMUX_R &= ~0xF000;       // Clear-all before writing
  ADC0_EMUX_R |= 0x5000;        // S3 is a Timer Triggered  Sample
  ADC0_SSMUX3_R &= ~0x000F;
  ADC0_SSMUX3_R += 9;           // set channel
  ADC0_SSCTL3_R = 0x0006;       // no Temp Meas, no Differential, Y int/EoS
  ADC0_IM_R |=    0x0008;       // Enable SS3 interrupts
  ADC0_ACTSS_R |= 0x0008;       // enable sample sequencer 3
  NVIC_PRI4_R   = (NVIC_PRI4_R&0xFFFF00FF)|0x00004000;
  NVIC_EN0_R    = (1 << 17);      // Enable ADC0 interrupt Per IRQ number in NVIC Table.
}

ADC3_Int_Handler(void)
{
  ADC0_ISC_R = 0x08;
  voltageIn = 3.3*((float)(ADC0_SSFIFO3_R & 0xFFF)/(4095));
}

void Hardware_Init(void)
{
  GPIO_Input_Config();
  ADC_Init();
  IntMasterEnable();
}
