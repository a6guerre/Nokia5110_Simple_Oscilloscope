/*
 * main.c
 */

/* Signal        (Nokia 5110) TM4C123G Pinout
   Reset         (RST, pin 1) --> PA7
   SSI0Fss       (CE,  pin 2) --> PA3
   Data/Command  (DC,  pin 3) --> PA6
   SSI0Tx        (Din, pin 4) --> PA5
   SSI0Clk       (Clk, pin 5) --> PA2
   3.3V          (Vcc, pin 6) --> +3.3 V
   not connected (BL,  pin 7) --> MCC
   Ground        (Gnd, pin 8)*/

#include <stdint.h>
#include "Nokia5110.h"
#include "tm4c123gh6pm.h"
#include "SPI_Driver.h"
#include "PLL_Init.h"

void Nokia5110_SetX(unsigned char newX)
{
  if(newX > 5)
  {
    return;
  }
  LCD_Write(COMMAND, 0x40|(newX));
}

void Nokia5110_SetY(unsigned char newY)
{
  if(newY > 5)
  {
    return;
  }
  LCD_Write(COMMAND, 0x40|(newY));
}

void Nokia5110_SetCursor(unsigned char newX, volatile unsigned char newY){
  if((newX > 11) || (newY > 5)){        // Error handling
    return;
  }
  LCD_Write(COMMAND, 0x80|(newX));     // setting bit 7 updates X-position
  LCD_Write(COMMAND, 0x40|(newY));     // setting bit 6 updates Y-position
}


void LCD_Write(xmit_type_t mode, char data)
{
  if(mode == COMMAND)
  {
    GPIO_PORTA_DATA_R &= ~0x40;
  }
  else if(mode == DATA)
  {
    GPIO_PORTA_DATA_R |= 0x40; 
  }
  while((SSI0_SR_R&SSI_SR_TFE)==0){};
  SSI0_DR_R = data;  // write to FIFO in order to xmit
  while((SSI0_SR_R&SSI_SR_RNE)==0){};
  data = SSI0_DR_R;  // read from FIFO to clear recv FIFO from useless data
}

void SetState_Reset()
{
  int delay;
  GPIO_PORTA_DATA_R &= ~RESET_PIN;            // Start from a clean state.
  for(delay=0; delay<10; delay=delay+1);      // delay minimum 100 ns
  GPIO_PORTA_DATA_R |= RESET_PIN;             // Drive pin low.
}

void Clear_Display()
{
  Nokia5110_SetCursor(0,0);
  int idx1, idx2;
  for(idx2 = 0; idx2 < 6; ++idx2)
  {
    for(idx1 = 0; idx1 < 84; ++idx1)
    {
      LCD_Write(DATA, 0x00);
    }
  }
  Nokia5110_SetCursor(0,0);
}

void Nokia5110_Config()
{ 
  SetState_Reset();
  LCD_Write(COMMAND, 0x21);
  LCD_Write(COMMAND, 0xC0);                // Contrast (Vop0 to Vop6) pp.14
  LCD_Write(COMMAND, 0x04);                // Temperature Coeffecient pp.14
  LCD_Write(COMMAND, 0x14);                // LCD bias mode
  LCD_Write(COMMAND, 
	    0x20 | (0<<PD | 0 << V | 0 << H)); // PD = 0, V = 0, H = 0
                                           // Regular Instruction set
  LCD_Write(COMMAND, 0x0C);                // Display Control normal

  // Set At Origin.
  LCD_Write(COMMAND, 0x80);
  LCD_Write(COMMAND, 0x40);

}

void InitializeNokia5110(void)
{
   Config_SPI();
   Nokia5110_Config();
}
