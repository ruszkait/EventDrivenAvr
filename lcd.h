/*
 * Initialize LCD controller.  Performs a software reset.
 */
void	LCD_Init(void);

/*
 * Send one character to the LCD.
 */
int	LCD_PutChar(char c);

/*
 *
 */
void LCD_MoveCursor(uint8_t Xpos, uint8_t Ypos);

void LCD_MoveWriteReg(uint8_t Xpos, uint8_t Ypos);

