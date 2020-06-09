/*
 * Display_Engine.c
 *
 *  Created on: May 7, 2020
 *      Author: a6gue
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "Nokia5110.h"
#include "tm4c123gh6pm.h"
#include "SPI_Driver.h"
#include "PLL_Init.h"
#include "ADC_VoltageIn.h"
#include "UART_Driver.h"
#include "Display_Engine.h"
#include "Button.h"
#include "linked_lists.h"
//#include "driverlib/interrupt.h"

#define DEBUG 1
#define NUM_ROWS 3
#define PADDING 3
#define MAX_WIDTH (84 - PADDING)
#define MAPPING_FACTOR(player_width) (float)(MAX_X - player_width)/(3.3)
int j;
int voltages[84];

uint8_t g_pixel_count;
int flag = 0;
int count;
int g_enemyKillCount;
int g_launched;
int g_has_launched;
int g_animate_counter;
int g_animate_counter_div = 16;
int g_enemy_counter = 3;
int g_propogate_counter;

float mapping;

List *bullet_list; 
List *enemy_bullets;

extern screenStruct Screen; // buffer stores the next image to be printed on the screenNokia5110.c
extern float voltageIn;
extern uint8_t debounce;

typedef struct enemy
{
  char posX;
  char posY;
  char row;
  char width;
  uint8_t visibility;
  const unsigned char* sprite;
}enemyObj;

typedef struct player
{
  char posX;
  char posY;
  char width;
  char height;
  char health;
  char destroyed;
  const unsigned char* sprite;
}playerShip;

typedef enum
{
  TITLE_SCREEN,
  GAME,
  END_GAME,
}GAME_STATES;

typedef struct title_screen
{
  const char *title_string;
}title_screen;

title_screen titleScreen = {"HEY"};

GAME_STATES current_state = TITLE_SCREEN;

enemyObj enemies[12];
enemyObj *available_enemies[4];
playerShip player;


typedef void (*call_back)(void);
call_back f;

void Timer0B_Handler(void)
{
  TIMER0_ICR_R |= 0x00000100;    // Acknowledge Interrupt Timer0B
  debounce = 1;                  // After expired time, it is ok to enable button interrupts
  TIMER0_CTL_R    &=  ~0x100;
}

void Launch_Bullet(void)
{
  switch (current_state)
  {
    case TITLE_SCREEN:
    {
        current_state = GAME;
        LaunchSpaceInvaders();
        break;
    }
    case GAME:
    {
        if(!(player.health == 0 || player.destroyed == 12))
        {
          appendAtEnd(bullet_list, bullet_list->size);
          bullet_node *current = bullet_list->tail;
          current->posX = player.posX + player.width/2;
          current->posY = player.posY - 8;
          current->visibility = 1;
          Nokia5110_PrintBMP_Bullet(current->posX, current->posY);
        }
        else
        {
          LaunchSpaceInvaders();
        }
    }
  }
}

void GPIO_PortE_Handler(void)
{
  GPIO_PORTE_ICR_R |= 0x02;   // Acknowledge Interrupt on PE1
  TIMER1_CTL_R &= ~0x00000001; // Make this atomic to prevent race conditions
  if(debounce)
  {
    //throw bullet
    int count;
    TIMER0_CTL_R    |=  0x100;
    Launch_Bullet();
    debounce = 0;
    g_launched = 1;
  }
  TIMER0_CTL_R    |=  0x100;
  TIMER1_CTL_R |= 0x00000001; // Resume screen refreshing
}

void Display_BMP(const char *data, size_t len)
{
  long width = data[18], height = data[22], i, j;
  int idx;
  j = data[10];  // Byte 10 contains the offset at the bottom left corner of the image
  for(idx = 0; idx < (width*height/2); ++idx)
  {
    LCD_Write(DATA, *data);
    ++data;
  }
}


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

void Shift_Right(void)
{
  int idx;
  int isFirstRow = 1;
  char temp, temp_2;
  char enemy_temp, enemy_temp_2;

  bullet_node *current = enemy_bullets->head;
  if(!(enemy_bullets->size == 0))
  {
    do
    {
      Nokia5110_ClearBMP_Bullet(current->posX, current->posY);
      current = current->next;
    }while(current != NULL);
  }

  for(idx = 0; idx < (MAX_X*(MAX_Y/8 - 1)); ++idx)
  {
      // We don't want to shift from top of next row
      // to current row
    if(isFirstRow)
    {
      temp = Screen.screen[idx + 1];
      enemy_temp = Screen.enemyIdx[idx + 1];

      Screen.screen[idx + 1] = Screen.screen[idx];
      Screen.enemyIdx[idx + 1] = Screen.enemyIdx[idx];

      isFirstRow = 0;
    }
    else if((idx%83) != 0)
    {
      temp_2 = Screen.screen[idx + 1];
      enemy_temp_2 = Screen.enemyIdx[idx + 1];

      Screen.screen[idx + 1] = temp;
      Screen.enemyIdx[idx + 1] = enemy_temp;

      temp = temp_2;
      enemy_temp = enemy_temp_2;
    }
    else
    {
      isFirstRow = 1;
    }
  }

  int x;

  for(x = 0; idx < 12; ++idx)
  {
    enemies[x].posX += 1;
  }
}

void Shift_Left(void)
{
  int idx;

  bullet_node *current = enemy_bullets->head;
  if(!(enemy_bullets->size == 0))
  {
    do
    {
      Nokia5110_ClearBMP_Bullet(current->posX, current->posY);
      current = current->next;
    }while(current != NULL);
  }

  for(idx = 0; idx < (MAX_X*(MAX_Y/8 - 1)); ++idx)
  {
    // We don't want to shift from top of next row
    // to current row
    if(idx%83 != 0)
    {
      Screen.screen[idx] = Screen.screen[idx + 1];
      Screen.enemyIdx[idx] = Screen.enemyIdx[idx + 1];
    }
  }
  int x;
  for(x = 0; idx < 12; ++idx)
  {
    enemies[x].posX -= 1;
  }
}

void Propogate_Enemy_Bullet(void)
{
  bullet_node *current = enemy_bullets->head;
  if(enemy_bullets->size == 0)
  {
    return;
  }
  do
  {
    // check if bullet is within bounds
    Nokia5110_ClearBMP_Bullet(current->posX, current->posY);
    ++(current->posY);
    Nokia5110_PrintBMP_Bullet(current->posX, current->posY);
    // if the bullet is no longer in the screen
    if(current->posY == 48)
    {
      // we need to remove
      Nokia5110_ClearBMP_Bullet(current->posX, current->posY);
      removeNode(&enemy_bullets, &current, current->idx);
      --enemy_bullets->size;
    }
    else{
      current = current->next;
    }
  }while(current != NULL);
}

void Propogate_Bullet(void)
{
  bullet_node *current = bullet_list->head;
  if(bullet_list->size == 0)
  {
    return;
  }
  do
  {
    if(current->visibility != 0)
    {
      // check if bullet is within bounds
      Nokia5110_ClearBMP_Bullet(current->posX, current->posY);
      --(current->posY);
      Nokia5110_PrintBMP_Bullet(current->posX, current->posY);
      // if the bullet is no longer in the screen
      if(current->posY == 0)
      {
        // we need to remove
        Nokia5110_ClearBMP_Bullet(current->posX, current->posY);
        removeNode(&bullet_list, &current, current->idx);
        --bullet_list->size;
      }
      else{
        current = current->next;
      }
    }
    else
    {
        current = current->next;
    }
  }while(current != NULL);
}

void Shift_Enemy_Bullets_Right(bullet_node *current, int shift_val)
{
  current = enemy_bullets->head;
  if(enemy_bullets->size == 0)
  {
    return;
  }
  do
  {
    Nokia5110_ClearBMP_Bullet(current->posX - 1, current->posY);
    Nokia5110_PrintBMP_Bullet(current->posX, current->posY);
    current = current->next;
  }while(current != NULL);
}

void Shift_Bullets_Right(bullet_node *current, int shift_val)
{
  current = bullet_list->head;
  if(bullet_list->size == 0)
  {
    return;
  }
  do
  {

    if (current->visibility != 0)
    {
      Nokia5110_ClearBMP_Bullet(current->posX - 1, current->posY);
      Nokia5110_PrintBMP_Bullet(current->posX, current->posY);
    }

    current = current->next;
  }while(current != NULL);
}

void Shift_Enemy_Bullets_Left(bullet_node *current, int shift_val)
{
  current = bullet_list->head;
  if(bullet_list->size == 0)
  {
    return;
  }
  do
  {
    if(current->visibility != 0)
    {
      Nokia5110_ClearBMP_Bullet(current->posX + 1, current->posY);
      Nokia5110_PrintBMP_Bullet(current->posX, current->posY);
    }
    current = current->next;
  }while(current != NULL);
}

void Shift_Bullets_Left(bullet_node *current, int shift_val)
{
  current = bullet_list->head;
  if(bullet_list->size == 0)
  {
    return;
  }
  do
  {
    if (current->visibility != 0)
    {
      Nokia5110_ClearBMP_Bullet(current->posX + 1, current->posY);
      Nokia5110_PrintBMP_Bullet(current->posX, current->posY);
    }
    current = current->next;
  }while(current != NULL);
}

int isWithinXBounds(unsigned char xpos)
{
  if((xpos >= player.posX) && (xpos <= (player.posX + player.width)))
  {
    return 1;
  }
  return 0;
}

int Detect_Enemy_Logic(unsigned char xpos, unsigned char ypos)
{
   if(ypos > 42)
   {
     if(isWithinXBounds(xpos)){
         return 1;
     }
   }
   return 0;
}

int Detect_Logic(unsigned char xpos, unsigned char ypos)
{
  unsigned short screenx, screeny;
  unsigned char shift;
  unsigned char mask;

  // bitmaps are encoded backwards, so start at the bottom left corner of the image
  screeny = ypos/8;
  screenx = xpos + SCREENW*screeny;
  shift = ypos%8;                 // row 0 to 7
  mask = 0x0F<<shift;             // now stores a mask 0x01 to 0x80
  //Screen.screen[screenx] |= mask;
  if(ypos > 35)
  {
      return 0;
  }

  if(((0x0F << shift)&0x100))      // If not all data fits in one byte
  {
     uint16_t temp = ((0x0F << shift) & ~0xFF);
     uint8_t  upper_data = (temp >> 8);
     screenx += SCREENW;
     if(!(screenx >= (MAX_X*MAX_Y/8)))
     {
       //Screen.screen[screenx] |= upper_data;
       if((Screen.screen[screenx] & 0xFF) != 0)
       {
         uint8_t enemyIdx = Screen.enemyIdx[screenx]; // which enemy is at the target?
         if (enemyIdx != 0 && (enemyIdx < 13))
         {
           enemies[enemyIdx - 1].visibility = 0;
           Nokia5110_PrintBMP(enemies[enemyIdx - 1].posX, enemies[enemyIdx - 1].posY, &ClearEnemy[0], 0,0);
           Nokia5110_PrintBMP((enemies[enemyIdx - 1].posX - 1), enemies[enemyIdx - 1].posY, &ClearEnemy[0], 0,0);
           Nokia5110_PrintBMP((enemies[enemyIdx - 1].posX + 1), enemies[enemyIdx - 1].posY, &ClearEnemy[0], 0,0);
           ++player.destroyed;
         }
         return 1;
       }
     }
  }
  else
  {
    if((Screen.screen[screenx] & 0xFF) != 0) // we do have a collision with a pre-existing sprite
    {
      if(ypos < 13)
      {
        screenx -= SCREENW;
      }
      uint8_t enemyIdx = Screen.enemyIdx[screenx]; // which enemy is at the target?
      if(enemyIdx != 0 && (enemyIdx < 13))
      {
        enemies[enemyIdx - 1].visibility = 0;
        Nokia5110_PrintBMP(enemies[enemyIdx - 1].posX, enemies[enemyIdx - 1].posY, &ClearEnemy[0], 0,0);
        Nokia5110_PrintBMP((enemies[enemyIdx - 1].posX - 1), enemies[enemyIdx - 1].posY, &ClearEnemy[0], 0,0);
        Nokia5110_PrintBMP((enemies[enemyIdx - 1].posX + 1), enemies[enemyIdx - 1].posY, &ClearEnemy[0], 0,0);
        ++player.destroyed;
      }
      return 1;
    }
  }
  return 0;
}

void Detect_Collision(void)
{
  bullet_node *current = bullet_list->head;
  bullet_node *temp;

  if(bullet_list->size != 0)
  {
    do
    {
      if (current->visibility != 0)
      {
        Nokia5110_ClearBMP_Bullet(current->posX, current->posY); // we do a collision detection using
                                                               // from what sprites are in same
                                                               // position as bullet
        //In order to not allow enemies to shift themselves
        //TIMER1_CTL_R &= ~0x00000001;
        if(Detect_Logic(current->posX, current->posY))
        {
          temp = current->next;
          //removeNode(&bullet_list, &current, current->idx);
          current->visibility = 0;
          //--(bullet_list->size);
          Nokia5110_ClearBMP_Bullet(current->posX, current->posY);
          current = temp;
        }
        else
        {
          Nokia5110_PrintBMP_Bullet(current->posX, current->posY);
          current = current->next;
        }
        // re-enable shifting
        //TIMER1_CTL_R |= 0x00000001;
      }
      else
      {
        current = current->next;
      }
    }while(current != NULL);
  }

  current = enemy_bullets->head;
  if(enemy_bullets->size != 0)
  {
    do
    {
      // Nokia5110_ClearBMP_Bullet(current->posX, current->posY); // we do a collision detection using
                                                                 // from what sprites are in same
                                                                 // position as bullet
      // In order to not allow enemies to shift themselves
      //TIMER1_CTL_R &= ~0x00000001;
      if(Detect_Enemy_Logic(current->posX, current->posY))
      {
        --(player.health);
        Nokia5110_ClearBMP_Bullet(current->posX, current->posY);
        temp = current->next;
        removeNode(&enemy_bullets, &current, current->idx);
        --enemy_bullets->size;
        current = temp;
      }
      else
      {
        // Nokia5110_PrintBMP_Bullet(current->posX, current->posY);
        current = current->next;
      }
      // re-enable shifting
      //TIMER1_CTL_R |= 0x00000001;
    }while(current != NULL);
  }
}

void End_Screen(void)
{
  //Clear_Display();
  if (player.health == 0)
  {
    Nokia5110_SetCursor(1,1);
    Nokia5110_OutString("GAME OVER!");
    Nokia5110_SetCursor(1,2);
    Nokia5110_OutString("You Loose");
    Nokia5110_SetCursor(1,3);
    Nokia5110_OutString("Try Again?");
  }
  else
  {
    Nokia5110_SetCursor(1,1);
    Nokia5110_OutString("YOU WIN!!");
    Nokia5110_SetCursor(0,2);
    Nokia5110_OutString("Play Again?");
    g_enemy_counter = 2;
    g_animate_counter_div = 8;
  }

}

void Oscillate_Enemies(void)
{
   static uint8_t toggle = 0;
   static uint8_t counter = 3;
   static uint8_t enemy_counter = 0;
   if(g_animate_counter%g_animate_counter_div == 0)
   {
     g_animate_counter = 0;
     if(!toggle)
     {
       Shift_Left();
       if(g_has_launched)
       {
         if(bullet_list->size != 0)
           Shift_Bullets_Right(bullet_list->head, 1);
       }
     }
     else
     {
       Shift_Right();
       if(g_has_launched)
       {
         if(bullet_list->size != 0)
           Shift_Bullets_Left(bullet_list->head, -1);
       }
     }

     ++counter;
     if(counter == 6)
     {
       toggle  = !toggle;
       counter = 0;
     }
     if(enemy_counter % g_enemy_counter == 0)
     {
       // Determine which enemy will fire next
       int num = (rand() % (4));
       if(enemies[num + 8].visibility)
       {
         appendAtEnd(enemy_bullets, enemy_bullets->size);
         bullet_node *current = enemy_bullets->tail;
         current->posX = enemies[num + 8].posX + enemies[num + 8].width/2;
         current->posY = enemies[num + 8].posY - 2;
         Nokia5110_PrintBMP_Bullet(current->posX, current->posY);
       }
       else if(enemies[num + 4].visibility)
       {
         appendAtEnd(enemy_bullets, enemy_bullets->size);
         bullet_node *current = enemy_bullets->tail;
         current->posX = enemies[num + 4].posX + enemies[num + 4].width/2;
         current->posY = enemies[num + 4].posY - 2;
         Nokia5110_PrintBMP_Bullet(current->posX, current->posY);

       }
       else if(enemies[num].visibility)
       {
         appendAtEnd(enemy_bullets, enemy_bullets->size);
         bullet_node *current = enemy_bullets->tail;
         current->posX = enemies[num].posX + enemies[num].width/2;
         current->posY = enemies[num].posY - 2;
         Nokia5110_PrintBMP_Bullet(current->posX, current->posY);
       }
       enemy_counter = 0;
     }
     ++enemy_counter;
   }
   ++g_animate_counter;
   // Display Updated results
   if(g_launched)
   {
     g_has_launched = 1;
     g_launched = 0;
   }
   // let's consider clearing and then re-drawing

   Nokia5110_PrintBMP(player.posX,player.posY, &ClearEnemy[0], 0,0);
   mapping = (float)(MAX_X - player.width)/(3.3);
   player.posX = (3.3 - voltageIn)*mapping;
   Nokia5110_PrintBMP(player.posX,player.posY, player.sprite, 0,0);

  // Nokia5110_SetCursor(0,5);
  // Nokia5110_OutString(player.health);

   if(g_propogate_counter%2 == 0)
   {
     Propogate_Bullet();
     Propogate_Enemy_Bullet();
     g_propogate_counter = 0;
   }
   ++g_propogate_counter;

   // Collision Detection
   Detect_Collision();
   Nokia5110_PrintScreen(&Screen.screen[0]);

  // Nokia5110_SetCursor(0,5);
  // Nokia5110_OutString(player.health);

   if(player.health == 0)
   {
     // done
     Nokia5110_PrintBMP(player.posX,player.posY, &ClearEnemy[0], 0,0);
     Clear_Display();
     f = &End_Screen;
   }
   if(player.destroyed == 12)
   {
     Nokia5110_PrintBMP(player.posX,player.posY, &ClearEnemy[0], 0,0);
     Clear_Display();
     f = &End_Screen;
   }
}

void Title_Screen_Animate(void)
{

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
   //TIMER1_ICR_R = TIMER_ICR_TATOCINT;
   // This needs to be atomic, or will affect buffering
  // IntMasterDisable();
   f();
  // IntMasterEnable();
   TIMER1_ICR_R = TIMER_ICR_TATOCINT;
}

void Timer1A_Config(void)
{
  SYSCTL_RCGC1_R |= SYSCTL_RCGC1_TIMER1;   // activate timer1
  TIMER1_CTL_R &= ~0x00000001;             // disable timer1A and timer 1B during setup
  TIMER1_CFG_R =   0x00000000;             // configure for 32-bit timer mode
  TIMER1_TAMR_R = 0x00000002;              // configure for one shot
  TIMER1_TAILR_R =  0x00100000;            // The rate at which we display a new pixel on LCD
  TIMER1_TAPR_R =   0xFF;                  // Divide by 80, so we have a 1 Mhz Clock
  TIMER1_ICR_R =  0x00000001;              // clear timer1A and timer 1B timeout flag
  TIMER1_IMR_R |= 0x00000101;              // arm timeout interrupt
  NVIC_PRI5_R = (NVIC_PRI5_R & 0xFFFF00FF)|0x00000400; // priority 2 for both timers
  NVIC_EN0_R |= 0x00200000;                // enable interrupt 19 in NVIC
  TIMER1_CTL_R |= 0x00000001;              // enable timer1A and timer 1B
}

void Initialize_Enemies(void)
{
  int idx1, idx2;
  int x = 0;
  for(idx1 = 0; idx1 < 3; ++idx1)
  {
    for(idx2 = 0; idx2 < 4; ++idx2)
    {
      enemies[x].posX = PADDING + (16*idx2);
      enemies[x].posY = 11*idx1  + 11;
      if(idx1 == 0)
      {
        enemies[x].width = SmallEnemy20PointB[18];
      }
      else
      {
        enemies[x].width = Enemy10Point1[18];
      }
      enemies[x].visibility = 1;
      if(idx1 > 1) // if this is the last row
      {
        available_enemies[x - 8] = &enemies[x];
      }
      ++x;
    }
  }
}

void Display_Enemies(void)
{
  int idx = 0;
  for(idx = 0; idx < 12; ++idx)
  {
    if (idx == 0 || idx < 4)
    {
      Nokia5110_PrintBMP(enemies[idx].posX, enemies[idx].posY, &SmallEnemy20PointB[0], 0, (idx + 1));
    }
    else
    {
      Nokia5110_PrintBMP(enemies[idx].posX, enemies[idx].posY, &Enemy10Point1[0], 0, (idx + 1));

    }
  }
}

void LaunchSpaceInvaders(void)
{
  Initialize_Samples();
  Initialize_Enemies();
  //DisplayObject(&Enemy10Point1[0], sizeof(Enemy10Point1)/sizeof(Enemy10Point1[0]));
  Display_Enemies();
  player.posX = 0;
  player.posY = 46;

  player.sprite = &PlayerShip0[0];
  player.width  = PlayerShip0[18];
  player.height = PlayerShip0[22];
  player.health = 5;
  player.destroyed = 0;

  Nokia5110_PrintBMP(player.posX,player.posY, player.sprite, 0, 0);
  Nokia5110_PrintScreen(Screen.screen);
  f = &Oscillate_Enemies;
}

void DrawTitleScreen()
{
  Nokia5110_SetCursor(1,0);
  Nokia5110_OutString("**********");
  Nokia5110_SetCursor(1,1);
  Nokia5110_OutString("* SPACE  *");
  Nokia5110_SetCursor(1,2);
  Nokia5110_OutString("*INVADERS*");
  Nokia5110_SetCursor(1,3);
  Nokia5110_OutString("**********");
  Nokia5110_SetCursor(0,4);
  Nokia5110_OutString("PRESS BUTTON");
  Nokia5110_SetCursor(2,5);
  Nokia5110_OutString("TO START");
}

void Display_Driver_Init(void)
{
  Timer1A_Config();
  DrawTitleScreen();
  f = &Title_Screen_Animate;
}

int main(void) {
  PLL_Init();
  Hardware_Init();
  UART_Init();
  InitializeNokia5110();
  Button_Config();
  Clear_Display();
  CreateList(&bullet_list);
  CreateList(&enemy_bullets);
  Display_Driver_Init();

  while(1)
  {
#ifdef DEBUG
    ++count;
    if(count%100000== 0)
    {
        UART_OutFloat(player.posX);
        UART_OutChar('\n');
        count = 0;
    }
#else
#endif
  }
  return 0;
}
