/*
 * Display_Engine.c
 *
 *  Created on: May 7, 2020
 *      Author: a6gue
 */
#include <stdint.h>
#include "Nokia5110.h"
#include "tm4c123gh6pm.h"
#include "SPI_Driver.h"
#include "PLL_Init.h"
#include "ADC_VoltageIn.h"
#include "UART_Driver.h"

//#define DEBUG 1
int j;
int voltages[84];

int g_pixel_count;
int flag = 0;
int count;

extern float voltageIn;

typedef void (*call_back)(void);
call_back f;

void Initialize_Samples(void)
{
  int i;
  float val = 0.0;
  for(i = 0; i < 84; ++i)
  {
    val += 0.0400;
    voltages[i] = 16*val;
  }
}

void Sample_Pattern(void)
{
  uint8_t idx;
  if(g_pixel_count < 84*6)
  {
    if(flag == 0) {
      LCD_Write(DATA, 0x02);
    }
    else
    {
      LCD_Write(DATA, 0x00);
    }
  }
  else
  {
    Nokia5110_SetCursor(0,0);
    g_pixel_count = 0;
    LCD_Write(DATA, 0x00);
    flag = 1;
  }
}

void Display_Output(void)
{
  unsigned char y_offset = voltages[g_pixel_count]/9;

  LCD_Write(COMMAND, 0x40|(y_offset));

  int raw_out = ((unsigned char)voltages[g_pixel_count])%9;
  if( (0x01 << ((unsigned char)raw_out) -1) == 0)
  {
    LCD_Write(DATA, 0x01);
  }
  else
  {
    LCD_Write(DATA, (0x01 << ((unsigned char)raw_out) - 1));
  }
  ++g_pixel_count;
  if(g_pixel_count == 84)
  {
    g_pixel_count = 0;
    Clear_Display();  
  } 

}

void Display_Voltage(void)
{

  int mapped_voltage = 16*voltageIn;
  unsigned char y_offset = mapped_voltage/9;

  //Nokia5110_SetCursor((char)g_pixel_count,5);
  LCD_Write(COMMAND, 0x40|(y_offset));

  int raw_out = ((unsigned char)mapped_voltage)%9;
  if( (0x01 << ((unsigned char)raw_out) -1) == 0)
  {
    LCD_Write(DATA, 0x01);
  }
  else
  {
    LCD_Write(DATA, (0x01 << ((unsigned char)raw_out) - 1));
  }
  ++g_pixel_count;
  if(g_pixel_count == 84)
  {
    g_pixel_count = 0;
    Clear_Display();
  }


}

void Timer1A_Handler(void)
{
   TIMER1_ICR_R = TIMER_ICR_TATOCINT;
   f();
}

void Timer1A_Config(void)
{
  SYSCTL_RCGC1_R |= SYSCTL_RCGC1_TIMER1;   // activate timer1
  TIMER1_CTL_R &= ~0x00000001;             // disable timer1A and timer 1B during setup
  TIMER1_CFG_R =   0x00000000;             // configure for 32-bit timer mode
  TIMER1_TAMR_R = 0x00000002;              // configure for one shot
  TIMER1_TAILR_R =  0x000FFFFF;            // The rate at which we display a new pixel on LCD
  TIMER1_TAPR_R =   0xFF;                  // Divide by 80, so we have a 1 Mhz Clock
  TIMER1_ICR_R =  0x00000001;              // clear timer1A and timer 1B timeout flag
  TIMER1_IMR_R |= 0x00000101;              // arm timeout interrupt
  NVIC_PRI5_R = (NVIC_PRI5_R & 0xFFFF00FF)|0x00000400; // priority 2 for both timers
  NVIC_EN0_R |= 0x00200000;                // enable interrupt 19 in NVIC
  TIMER1_CTL_R |= 0x00000001;              // enable timer1A and timer 1B
}
void Display_Driver_Init(void)
{
  Timer1A_Config();
  f = &Display_Voltage;
  Initialize_Samples();
}

int main(void) {
  PLL_Init();
  Hardware_Init();
  UART_Init();
  InitializeNokia5110();
  Clear_Display();
  Display_Driver_Init();

  while(1)
  {
#ifdef DEBUG
    ++count;
    if(count%100000== 0)
    {
        UART_OutFloat(voltageIn);
        count = 0;
    }
#else
#endif
  }
  return 0;
}
