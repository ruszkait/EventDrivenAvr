// ***********************************************************
// Project:
// Author:
// Module description:
// ***********************************************************

#include <avr\io.h>              // Most basic include files
#include <avr\interrupt.h>       // Add the necessary ones
#include <stdint.h>
#include <stdbool.h>
#include <util\atomic.h>

#include "config.h"
#include "messagequeue.h"
#include "buttons.h"
#include "messages.h"
#include "timer.h"
#include "heap.h"
#include "macroes.h"
#include "spi.h"
#include "main.h"

// 0.25K Heap
char globalHeap[0x100];

// ***********************************************************
//
//
void ButtonPressed(struct QueueElementT* message)
{
	struct KeyPressParam* keyPressData = (struct KeyPressParam*)message->m_Data;
	enum KeyCodeT keyCode = keyPressData->m_keyCode;

	// Create the a LED message
	struct QueueElementT ledSwitchMessage;
	ledSwitchMessage.m_Command = &LedSwitch;
	
	struct LEDMessageParam ledSwitchData;
	ledSwitchData.m_LEDId = keyCode;
	ledSwitchData.m_SwitchOn = true;
	ledSwitchMessage.m_Data = (QueueDataT*)&ledSwitchData;
	ledSwitchMessage.m_DataSize = sizeof(struct LEDMessageParam);

	// Add the message to the queue
	MessageQueue_Push(&ledSwitchMessage);
}

// ***********************************************************
//
//
void ButtonReleased(struct QueueElementT* message)
{
	struct KeyPressParam* keyPressData = (struct KeyPressParam*)message->m_Data;
	enum KeyCodeT keyCode = keyPressData->m_keyCode;

	// Create the a LED message
	struct QueueElementT ledSwitchMessage;
	ledSwitchMessage.m_Command = &LedSwitch;
	
	struct LEDMessageParam ledSwitchData;
	ledSwitchData.m_LEDId = keyCode;
	ledSwitchData.m_SwitchOn = false;
	ledSwitchMessage.m_Data = (QueueDataT*)&ledSwitchData;
	ledSwitchMessage.m_DataSize = sizeof(struct LEDMessageParam);
	
	// Add the message to the queue
	MessageQueue_Push(&ledSwitchMessage);
}

// ***********************************************************
//
//
void FlashKeepAliveLED(struct QueueElementT* message)
{
	static bool KeepAliveLedOn = false;

	// Toggle the LED state
	KeepAliveLedOn = !KeepAliveLedOn;
	
	// Send a LED switching message
	
	// Create the a LED message
	struct QueueElementT ledMessage;
	ledMessage.m_Command = &LedSwitch;
	
	struct LEDMessageParam data;
	data.m_LEDId = 0;
	data.m_SwitchOn = KeepAliveLedOn;
	ledMessage.m_Data = (QueueDataT*)&data;
	ledMessage.m_DataSize = sizeof(struct LEDMessageParam);

	// Add the message to the queue
	MessageQueue_Push(&ledMessage);


	SPI_Send(0xbb);
}

// ***********************************************************
//
//
void CPU_Init(void)
{
	// Set the MCUCR bit SE -> Sleep enabled
	MCUCR |= 1 << SE;

	sei();
}

// ***********************************************************
//
//
void LED_Init(void)
{
	// Set LED port direction to output
	DDR(LED_PORT) = 0xff;
	// Turn the LED off
	PORT(LED_PORT) = 0xff;
}

// ***********************************************************
//
//
void Keyboard_Init(void)
{
	// Set switch port direction to input
	DDR(BUTTON_PORT) = 0;
	// Enable Pull up resistor
	PORT(BUTTON_PORT) = 0xff;
}

// ***********************************************************
//
//
void SystemStart(struct QueueElementT* message)
{
	// Add the key scanning timer
	struct QueueElementT timerMessage;
	timerMessage.m_Command = &MonitorAllButtons;
	timerMessage.m_DataSize = 0;
	Timer_Add(70, TIMER_INFINITE_REPEAT, &timerMessage);

	// Add the keepalive LED timer
	timerMessage.m_Command = &FlashKeepAliveLED;
	timerMessage.m_DataSize = 0;
	Timer_Add(30, TIMER_INFINITE_REPEAT, &timerMessage);
}

// ***********************************************************
//
//
void LedSwitch(struct QueueElementT* message)
{
	struct LEDMessageParam* data = (struct LEDMessageParam*)message->m_Data;
	uint8_t p_LEDId = data->m_LEDId;
	bool p_switchOn = data->m_SwitchOn;

	// The first data byte shows if the LED
	// is to be turned on/off
	if (p_switchOn)
	{
		// Setting port to LOW will turn the LED on
		PORT(LED_PORT) &= ~(1 << p_LEDId);
	}
	else
	{
		// Setting port to HIGH will turn the LED off
		PORT(LED_PORT) |= (1 << p_LEDId);
	}
}

// ***********************************************************
//
//
void Init(void)
{
	// Initiliaze the message queue
	MessageQueue_Init();

	// Create the start message
	struct QueueElementT initMessage;
	initMessage.m_Command = &SystemStart;
	initMessage.m_DataSize = 0;

	MessageQueue_Push(&initMessage);

	Heap_Init(globalHeap, sizeof(globalHeap));
	Timer_Init();
	LED_Init();
	Keyboard_Init();
	CPU_Init();
	SPI_Init();
}


void heaptest(void)
{
	Heap_Init(globalHeap, sizeof(globalHeap));

	Heap_RedirectionIndexT heapHandle = Heap_Alloc(globalHeap, 10);

	uint8_t* buffer = (uint8_t*)Heap_Pin(globalHeap, heapHandle);
	for (int i = 0; i < 10; ++i)
	{
		buffer[i] = 0x11*(i+1);
	}
	Heap_UnPin(globalHeap, heapHandle);

	Heap_RedirectionIndexT heapHandle2 = Heap_Alloc(globalHeap, 10);

	buffer = (uint8_t*)Heap_Pin(globalHeap, heapHandle2);
	for (int i = 0; i < 10; ++i)
	{
		buffer[i] = 0x10*(i+1);
	}
	Heap_UnPin(globalHeap, heapHandle2);


	Heap_RedirectionIndexT heapHandle3 = Heap_Alloc(globalHeap, 10);

	buffer = (uint8_t*)Heap_Pin(globalHeap, heapHandle3);
	for (int i = 0; i < 10; ++i)
	{
		buffer[i] = 0x01*(i+1);
	}
	Heap_UnPin(globalHeap, heapHandle3);
	
	Heap_Free(globalHeap, heapHandle2);

	Heap_CompactAllBlocks(globalHeap);
	Heap_CompactAllBlocks(globalHeap);
	Heap_CompactAllBlocks(globalHeap);

	Heap_Free(globalHeap, heapHandle);
	Heap_Free(globalHeap, heapHandle3);

	Heap_CompactAllBlocks(globalHeap);
	Heap_CompactAllBlocks(globalHeap);
	Heap_CompactAllBlocks(globalHeap);


	heapHandle = Heap_Alloc(globalHeap, 10);

	buffer = (uint8_t*)Heap_Pin(globalHeap, heapHandle);
	for (int i = 0; i < 10; ++i)
	{
		buffer[i] = 0x11*(i+1);
	}
	Heap_UnPin(globalHeap, heapHandle);


}

// ***********************************************************
// 
//
int main(void)
{
	heaptest();

	// Initiliaze the system
	Init();

	struct QueueElementT message;

	// The main loop
	while(1)
	{
		// Set data pointer to null, this way the pop will not copy the
		// data, only fills in the data size and the command code
		message.m_Data = 0;

		// Check if we have message in the queue
		// and if so then fetch it
		if (!MessageQueue_Pop(&message))
		{
			// Maintain the heap before going to sleep
			Heap_CompactAllBlocks(globalHeap);

			// Start an atomic block
			cli();

			// A final check before going to sleep
			if (!MessageQueue_IsEmpty())
			{
				// Leave the atomic section
				sei();
				continue;
			}

			// Enable interrupts and go to sleep
			// After sei then next instruction (sleep) will be
			// executed before any pending interrupts
			asm(
				"sei" "\n\t"
				"sleep" "\n\t"
				);


			// An interrupt woken the CPU up
			// The ISR was executed and now we are here
			// Check if the interrupt put a message into the message queue
			continue;
		}
		
		if (0 != message.m_DataSize)
		{
			// The message needs a buffer to be able to really pop it
			QueueDataT messageData[message.m_DataSize];
			message.m_Data = (QueueDataT*)&messageData;
			MessageQueue_Pop(&message);

			// Dispatch to callback
			message.m_Command(&message);
		}
		else
		{
			// Message has no data part, so it was already popped

			// Dispatch to callback
			message.m_Command(&message);
		}
	}
}










