#ifndef CONFIG
#define CONFIG

/* CPU frequency */
#define F_CPU 16000000UL

/* Timer */
#define TIMER_CONTROL_BLOCK_BUFFER_SIZE 32

/* Message queue in bytes, must be of power of 2*/
#define MAIN_MESSAGE_QUEUE_SIZE 32

/* HD44780 LCD port connections */
#define HD44780_PORT B
#define HD44780_RS PORT6
#define HD44780_RW PORT4
#define HD44780_E  PORT5
#define HD44780_D4 PORT0
#define HD44780_D5 PORT1
#define HD44780_D6 PORT2
#define HD44780_D7 PORT3

/* LED config */
#define LED_PORT C

/* Keyboard config */
#define BUTTON_PORT D
#define BUTTON_PRESSING_THRESHOLD 3

#endif

