#include <stdint.h>
#include "buttons.h"
#include "messagequeue.h"
#include "messages.h"
#include "config.h"
#include "main.h"
#include <avr\io.h>              // Most basic include files
#include "macroes.h"

uint8_t keyPressedCounter [KEY_MAX];
uint8_t keyReleasedCounter [KEY_MAX];

// ***********************************************************
//
//
void MonitorAllButtons(struct QueueElementT* message)
{
	// Key and pin association
	const uint8_t switchBits = ~PIN(BUTTON_PORT);
	MonitorButton(switchBits & (1 << PINBIT(BUTTON_PORT, 0)), KEY_0);
	MonitorButton(switchBits & (1 << PINBIT(BUTTON_PORT, 1)), KEY_1);
	MonitorButton(switchBits & (1 << PINBIT(BUTTON_PORT, 2)), KEY_2);
	MonitorButton(switchBits & (1 << PINBIT(BUTTON_PORT, 3)), KEY_3);
	MonitorButton(switchBits & (1 << PINBIT(BUTTON_PORT, 4)), KEY_4);
	MonitorButton(switchBits & (1 << PINBIT(BUTTON_PORT, 5)), KEY_5);
	MonitorButton(switchBits & (1 << PINBIT(BUTTON_PORT, 6)), KEY_6);
}

// ***********************************************************
//
//
void MonitorButton(bool switchClosed, enum KeyCodeT keyCode)
{
	if (switchClosed)
	{
			// When the counter reached 1 then generate an event
		if (1 == keyPressedCounter[keyCode])
		{
			struct QueueElementT buttonPressedMessage;
			buttonPressedMessage.m_Command = &ButtonPressed;
			struct KeyPressParam data;
			data.m_keyCode = keyCode;
			buttonPressedMessage.m_Data = (QueueDataT*)&data;
			buttonPressedMessage.m_DataSize = sizeof(struct KeyPressParam);

			// Add the message to the queue
			MessageQueue_Push(&buttonPressedMessage);
		}

		// When the counter reached 0 then stay at 0 to prevent
		// continously generating the event
		if (0 < keyPressedCounter[keyCode])
		{
			--keyPressedCounter[keyCode];
		}

		// Clear the other state counter
		keyReleasedCounter[keyCode] = BUTTON_PRESSING_THRESHOLD;
	}
	else
	{
		if (1 == keyReleasedCounter[keyCode])
		{
			struct QueueElementT buttonReleasedMessage;
			buttonReleasedMessage.m_Command = &ButtonPressed;
			struct KeyPressParam data;
			data.m_keyCode = keyCode;
			buttonReleasedMessage.m_Data = (QueueDataT*)&data;
			buttonReleasedMessage.m_DataSize = sizeof(struct KeyPressParam);

			// Add the message to the queue
			MessageQueue_Push(&buttonReleasedMessage);
		}

		if (0 < keyReleasedCounter[keyCode])
		{
			--keyReleasedCounter[keyCode];
		}

		// Clear the other state counter		
		keyPressedCounter[keyCode] = BUTTON_PRESSING_THRESHOLD;
	}
}

// ***********************************************************
//
//
char KeyCodeToASCII(enum KeyCodeT keyCode)
{
	switch(keyCode)
	{
		case KEY_0:
			return '0';
		case KEY_1:
			return '1';
		case KEY_2:
			return '2';
		case KEY_3:
			return '3';
		case KEY_4:
			return '4';
		case KEY_5:
			return '5';
		case KEY_6:
			return '6';
		case KEY_7:
			return '7';
		default:
			return '\0';
	}
}



