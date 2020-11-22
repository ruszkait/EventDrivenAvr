#include "config.h"

#include <stdbool.h>
#include <stdint.h>

#include <avr/io.h>

#include <util/delay.h>

#include "hd44780.h"
#include "lcd.h"

void LCD_Init(void)
{
  hd44780_init();

  // Clear the display.
  hd44780_wait_ready();
  hd44780_outcmd(HD44780_CLR);
}

uint8_t LCD_GetRowStartAddress(uint8_t Ypos)
{
	switch (Ypos)
	{
		default:
		case 0:
			return 0x00;
		break;
		case 1:
			return 0x40;
		break;
		case 2:
			return 0x14;
		break;
		case 3:
			return 0x54;
		break;
	}
}

void LCD_MoveWriteReg(uint8_t Xpos, uint8_t Ypos)
{
   hd44780_wait_ready();
	hd44780_outcmd(HD44780_DDADDR(Xpos + LCD_GetRowStartAddress(Ypos)));
}

void LCD_MoveCursor(uint8_t Xpos, uint8_t Ypos)
{
   hd44780_wait_ready();
   hd44780_outcmd(HD44780_HOME);
	
	uint8_t bufferAddress = Xpos + LCD_GetRowStartAddress(Ypos);

	for (uint8_t x = bufferAddress; 0 != x; --x)
	{
      hd44780_wait_ready();
      hd44780_outcmd(HD44780_SHIFT(0/* Move cursor*/, 1 /*Move right*/));
	}
}

/*
 * Send character c to the LCD display.  After a '\n' has been seen,
 * the next character will first clear the display.
 */
int LCD_PutChar(char c)
{
  static bool nl_seen;

  if (nl_seen && c != '\n')
    {
      /*
       * First character after newline, clear display and home cursor.
       */
      hd44780_wait_ready();
      hd44780_outcmd(HD44780_CLR);
      hd44780_wait_ready();
      hd44780_outcmd(HD44780_HOME);
      hd44780_wait_ready();
      hd44780_outcmd(HD44780_DDADDR(0));

      nl_seen = false;
    }
  if (c == '\n')
    {
      nl_seen = true;
    }
  else
    {
      hd44780_wait_ready();
      hd44780_outdata(c);
    }

  return 0;
}



