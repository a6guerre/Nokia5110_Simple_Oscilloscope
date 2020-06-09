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

screenStruct Screen;

//********Nokia5110_OutChar*****************
// Print a character to the Nokia 5110 48x84 LCD.  The
// character will be printed at the current cursor position,
// the cursor will automatically be updated, and it will
// wrap to the next row or back to the top if necessary.
// One blank column of pixels will be printed on either side
// of the character for readability.  Since characters are 8
// pixels tall and 5 pixels wide, 12 characters fit per row,
// and there are six rows.
// inputs: data  character to print
// outputs: none
// assumes: LCD is in default horizontal addressing mode (V = 0)
void Nokia5110_OutChar(unsigned char data){
  int i;
  LCD_Write(DATA, 0x00);                 // blank vertical line padding
  for(i=0; i<5; i=i+1){
    LCD_Write(DATA, ASCII[data - 0x20][i]);
  }
  LCD_Write(DATA, 0x00);                 // blank vertical line padding
}


//********Nokia5110_OutString*****************
// Print a string of characters to the Nokia 5110 48x84 LCD.
// The string will automatically wrap, so padding spaces may
// be needed to make the output look optimal.
// inputs: ptr  pointer to NULL-terminated ASCII string
// outputs: none
// assumes: LCD is in default horizontal addressing mode (V = 0)
void Nokia5110_OutString(char *ptr){
  while(*ptr){
    Nokia5110_OutChar((unsigned char)*ptr);
    ptr = ptr + 1;
  }
}

void Nokia5110_PrintScreen(const char *data)
{
  int idx;
  Nokia5110_SetCursor(0,0);
  for(idx = 0; idx < (MAX_X*MAX_Y/8); ++idx)
  {
    LCD_Write(DATA, data[idx]);
  }
}

void Nokia5110_PrintBMP_Bullet(unsigned char xpos, unsigned char ypos){
    unsigned short screenx, screeny;
    unsigned char shift;
    unsigned char mask;

    screeny = ypos/8;
    screenx = xpos + SCREENW*screeny;
    shift = ypos%8;                 // row 0 to 7
    mask = 0x0F<<shift;             // now stores a mask 0x01 to 0x80
    Screen.screen[screenx] |= mask;
    if(((0x0F << shift)&0x100))      // If not all data fits in one byte
    {
       uint16_t temp = ((0x0F << shift) & ~0xFF);
       uint8_t  upper_data = (temp >> 8);
       screenx += SCREENW;
       if(!(screenx >= (MAX_X*MAX_Y/8)))
       {
         Screen.screen[screenx] |= upper_data;
       }
    }
}

void Nokia5110_ClearBMP_Bullet(unsigned char xpos, unsigned char ypos){
    unsigned short screenx, screeny;
    unsigned char shift;
    unsigned char mask;

    // bitmaps are encoded backwards, so start at the bottom left corner of the image
    screeny = ypos/8;
    screenx = xpos + SCREENW*screeny;
    shift = ypos%8;                // row 0 to 7
    mask = 0x0F<<shift;             // now stores a mask 0x01 to 0x80
    Screen.screen[screenx] &= ~mask;
    if(((0x0F << shift)&0x100))      // If not all data fits in one byte
    {
       uint16_t temp = ((0x0F << shift) & ~0xFF);
       uint8_t  upper_data = (temp >> 8);
       screenx += SCREENW;
       if(!(screenx >= (MAX_X*MAX_Y/8)))
       {
         Screen.screen[screenx] &= ~upper_data;
       }
    }
}


void Nokia5110_PrintBMP(unsigned char xpos, unsigned char ypos, const unsigned char *ptr, unsigned char threshold, uint8_t enemyIdx){
  long width = ptr[18], height = ptr[22], i, j;
  unsigned short screenx, screeny;
  unsigned char mask;
  // check for clipping
  if((height <= 0) ||              // bitmap is unexpectedly encoded in top-to-bottom pixel order
     ((width%2) != 0) ||           // must be even number of columns
     ((xpos + width) > SCREENW) || // right side cut off
     (ypos < (height - 1)) ||      // top cut off
     (ypos > SCREENH))           { // bottom cut off
    return;
  }
  if(threshold > 14){
    threshold = 14;             // only full 'on' turns pixel on
  }
  // bitmaps are encoded backwards, so start at the bottom left corner of the image
  screeny = ypos/8;
  screenx = xpos + SCREENW*screeny;
  //Screen.enemyIdx[screenx] = enemyIdx;
  mask = ypos%8;                // row 0 to 7
  mask = 0x01<<mask;            // now stores a mask 0x01 to 0x80
  j = ptr[10];                  // byte 10 contains the offset where image data can be found
  for(i=1; i<=(width*height/2); i=i+1){
    // the left pixel is in the upper 4 bits
    if(((ptr[j]>>4)&0xF) > threshold){
      Screen.screen[screenx] |= mask;
      Screen.enemyIdx[screenx] = enemyIdx;
    } else{
      Screen.screen[screenx] &= ~mask;
    }
    screenx = screenx + 1;
    // the right pixel is in the lower 4 bits
    if((ptr[j]&0xF) > threshold){
      Screen.screen[screenx] |= mask;
      Screen.enemyIdx[screenx] = enemyIdx;
    } else{
      Screen.screen[screenx] &= ~mask;
    }
    screenx = screenx + 1;
    j = j + 1;
    if((i%(width/2)) == 0){     // at the end of a row
      if(mask > 0x01){
        mask = mask>>1;
      } else{
        mask = 0x80;
        screeny = screeny - 1;
      }
      screenx = xpos + SCREENW*screeny;
      //Screen.enemyIdx[screenx] = enemyIdx;
      // bitmaps are 32-bit word aligned
      switch((width/2)%4){      // skip any padding
        case 0: j = j + 0; break;
        case 1: j = j + 3; break;
        case 2: j = j + 2; break;
        case 3: j = j + 1; break;
      }
    }
  }
}

void Nokia5110_SetVertAddr(void)
{
    LCD_Write(COMMAND,
           0x20 | 0x02); // PD = 0, V = 1, H = 0
}

void Nokia5110_SetHorizAddr(void)
{
    LCD_Write(COMMAND,
           0x20 | (0<<PD | 0 << V | 0 << H)); // PD = 0, V = 1, H = 0
}

void Nokia5110_SetX(unsigned char newX)
{
  if(newX > 5)
  {
    return;
  }
  LCD_Write(COMMAND, 0x80|(newX));
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
  LCD_Write(COMMAND, 0x80|(newX*7));     // setting bit 7 updates X-position
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
