/*
 * ADC_VoltageIn.h
 *
 *  Created on: May 9, 2020
 *      Author: a6gue
 */

#ifndef ADC_VOLTAGEIN_H_
#define ADC_VOLTAGEIN_H_

#include "tm4c123gh6pm.h"
#include "UART_Driver.h"
#include <stdint.h>

#define FREQ_TO_TICKS(sample_freq, clock_freq)   (clock_freq/sample_freq)
/*
 * This function configures the GPIO Port and Pin that will
 * be used for reading voltage input by the ADC Sampler.
 */
void GPIO_Input_Config(void);
/*
 * Configures the timer that will be driving the period of the
 * ADC Samples
 */
void Trigger_Timer_Config(uint32_t load_val);
void ADC_Init(void);
void ADC3_Int_Handler(void);
void Hardware_Init(void);

#endif /* ADC_VOLTAGEIN_H_ */
