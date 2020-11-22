#ifndef BUTTONS
#define BUTTONS

#include <stdbool.h>

enum KeyCodeT
{
	KEY_0,
	KEY_1,
	KEY_2,
	KEY_3,
	KEY_4,
	KEY_5,
	KEY_6,
	KEY_7,
	KEY_8,
	KEY_9,
	KEY_A,
	KEY_B,
	KEY_C,
	KEY_D,
	KEY_MAX
};

// Monitors one button
// Parameters: switchClosed if the button switch was closed
// Parameters: keyCode the code of the monitored button
// Returns: none
void MonitorButton(bool switchClosed, enum KeyCodeT keyCode);

// Keycode -> ASCII conversion
// Parameters: the key code
// Returns: ASCII code
char KeyCodeToASCII(enum KeyCodeT keyCode);

#endif
