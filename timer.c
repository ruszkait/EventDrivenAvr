#include <avr\io.h>              // Most basic include files
#include <avr\interrupt.h>       // Add the necessary ones
#include <util\atomic.h>

#include "timer.h"
#include "config.h"
#include "messagequeue.h"

//
// Data types
//
struct TimerControlBlockT
{
	int32_t 		m_TicksTillNextFiring;
	int32_t 		m_FiringPeriodTicks;
	uint8_t			m_RepeatCount;
	QueueCommandT	m_Command;
	uint8_t			m_CommandDataLength;
};

static uint8_t timerBuffer[TIMER_CONTROL_BLOCK_BUFFER_SIZE];
static uint8_t numberOfTimerBlocks = 0;

#define TIMER_PRESCALER 1024

void Timer_AgeTimerBlocks(const uint16_t ellapsedTicks);

// ***********************************************************
//
//
void Timer_Init(void)
{
	// Set CTC (Clear Timer on Compare Match) mode and the timer prescaler clk/1024
	// 16MHz -> 16 count = 1 ms
	TCCR1B = (1 << WGM12 | 1 << CS12 | 1 << CS10);

	// Counter 1
	TCNT1 = 0;
	
	// Counter limit 1/A
	OCR1A = 16;

	// Counter 1/A limit match interrupt
	TIMSK |= 1 << OCIE1A;
}

// ***********************************************************
//
//
void Timer_CollectExpiredTimerBlocks(void)
{
	register uint8_t *collectDstPtr = timerBuffer;
	register uint8_t *collectSrcPtr = timerBuffer;
	bool hadCollectedBlock = false;

	for (register uint8_t timerBlockIter = numberOfTimerBlocks; 0 != timerBlockIter; --timerBlockIter)
	{
		// Is this timer control block is invalid
		if (0 == ((struct TimerControlBlockT *)collectSrcPtr)->m_RepeatCount)
		{
			// This Timer control block is to be collected
			hadCollectedBlock = true;

			// Move to the next Dst block
			collectSrcPtr += sizeof(struct TimerControlBlockT) + ((struct TimerControlBlockT *)collectSrcPtr)->m_CommandDataLength;
			
			// unregister the invalid block
			--numberOfTimerBlocks;
		}
		else
		{
			// The current source block is valid
			
			// If there were invalid blocks before the copy this in place of the invalid ones
			if (hadCollectedBlock)
			{
				// Move the current Src block to the empty space at Dst
				for (register uint8_t moveBlockIter = sizeof(struct TimerControlBlockT) + ((struct TimerControlBlockT *)collectSrcPtr)->m_CommandDataLength;
						0 != moveBlockIter; --moveBlockIter)
				{
					*collectDstPtr = *collectSrcPtr;
					++collectDstPtr;
					++collectSrcPtr;
				}
			}
			else
			{
				// Step to the next block as there was nothing to collect
				collectSrcPtr += sizeof(struct TimerControlBlockT) + ((struct TimerControlBlockT *)collectSrcPtr)->m_CommandDataLength;

				// The Dst pointer shall move together with the Src
				collectDstPtr = collectSrcPtr;
			}
		}
	}
}

// ***********************************************************
//
//
void Timer_InsertTimerBlock(int32_t delayMs, uint8_t repeat, struct QueueElementT *timeredMessage)
{
	// Apply the ellapsed ticks to the active timers
	// So from this point on the running HW counter counts also for this new timer
	uint16_t ellapsedTicks = TCNT1;
	TCNT1 = 0;
	Timer_AgeTimerBlocks(ellapsedTicks);

	register uint8_t *bufferPtr = timerBuffer;
	// Find the last item in the list
	for (register uint8_t timerBlockiter = numberOfTimerBlocks; 0 != timerBlockiter; --timerBlockiter)
	{
		// buffer pointer shall jump the size of the control block + the datablock
		bufferPtr += sizeof(struct TimerControlBlockT) + ((struct TimerControlBlockT *)bufferPtr)->m_CommandDataLength;
	}
	
	// Fill in the control block
	register struct TimerControlBlockT *newTimerControlBlock = (struct TimerControlBlockT *)bufferPtr;
	// The Original tick counter
	newTimerControlBlock->m_FiringPeriodTicks = (delayMs * (uint32_t)(F_CPU / 1000)) / (uint32_t)TIMER_PRESCALER;
	newTimerControlBlock->m_TicksTillNextFiring = newTimerControlBlock->m_FiringPeriodTicks;
	newTimerControlBlock->m_RepeatCount = repeat;
	newTimerControlBlock->m_Command = timeredMessage->m_Command;
	newTimerControlBlock->m_CommandDataLength = timeredMessage->m_DataSize;

	// Move the buffer pointer to the command data part
	bufferPtr += sizeof(struct TimerControlBlockT);

	// Copy the command data part to the time buffer
	QueueDataT *commandDataPtr = timeredMessage->m_Data;
	for (uint8_t commandDataIter = newTimerControlBlock->m_CommandDataLength; 0 != commandDataIter; --commandDataIter)
	{
		// copy byte by byte
		*bufferPtr++ = *commandDataPtr++;
	}
	
	// Register the new timer
	++numberOfTimerBlocks;

	// Age the timer with the ellapsed ticks and reschedule the timers because of the new timer
	ellapsedTicks = TCNT1;
	TCNT1 = 0;
	Timer_AgeTimerBlocks(ellapsedTicks);
}

// ***********************************************************
//
//
void Timer_AgeTimerBlocks(const uint16_t ellapsedTicks)
{
	uint8_t *bufferPtr = timerBuffer;

	// The highest value of the lowest tick count can be the maximum value of the timer counter (16bit)
	int32_t lowestTickCount = UINT16_MAX;

	for (register uint8_t timerBlockIter = numberOfTimerBlocks; 0 != timerBlockIter; --timerBlockIter)
	{
		register struct TimerControlBlockT *timerControlBlock = (struct TimerControlBlockT *)bufferPtr;

		// Store the ellapsed time

		// If the round counter is not 0 then this timer is still active
		if (0 != timerControlBlock->m_RepeatCount)
		{
			// Subtract the ellapsed ticks
			timerControlBlock->m_TicksTillNextFiring -= ellapsedTicks;

			// If the tick counter has run out then fire the timer event
			if (0 >= timerControlBlock->m_TicksTillNextFiring)
			{
				// Send the corresponding message to the queue
				struct QueueElementT timerMessage;
				timerMessage.m_Command = timerControlBlock->m_Command;
				timerMessage.m_DataSize = timerControlBlock->m_CommandDataLength;
				timerMessage.m_Data = bufferPtr + sizeof(struct TimerControlBlockT);
				MessageQueue_Push(&timerMessage);

 				if (TIMER_INFINITE_REPEAT != timerControlBlock->m_RepeatCount)
 				{
					// The timer was fired, so decrease the round counter
					--timerControlBlock->m_RepeatCount;
				}

				// If we have another round then refill the timer counter
				if (0 != timerControlBlock->m_RepeatCount)
				{
					timerControlBlock->m_TicksTillNextFiring += timerControlBlock->m_FiringPeriodTicks;
				}
			}

			// Find the smallest tick count to schedule the next waiting
			
			// If the timer is still valid and the remainig ticks are the smallest so far
			if (0 != timerControlBlock->m_RepeatCount &&	timerControlBlock->m_TicksTillNextFiring < lowestTickCount)
			{
				lowestTickCount = timerControlBlock->m_TicksTillNextFiring;
			}
		}

		// Jump to the next timer control block
		// buffer pointer shall jump the size of the control block + the datablock
		bufferPtr += sizeof(struct TimerControlBlockT) + ((struct TimerControlBlockT *)bufferPtr)->m_CommandDataLength;
	}

	// Set the counter limit
	const uint16_t lowestTickCount16 = (uint16_t)lowestTickCount;
	if (TCNT1 >= lowestTickCount16)
	{
		// The desired timer counter value has already passed, so
		// set our limit a bit above the counter to make sure that
		// the timer interrupt will hit.
		OCR1A = TCNT1 + 2;
	}
	else
	{
		OCR1A = lowestTickCount16;
	}
}


// ***********************************************************
//
//
void Timer_Add(int32_t delayMs, uint8_t repeat, struct QueueElementT *timeredMessage)
{
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		Timer_CollectExpiredTimerBlocks();
		Timer_InsertTimerBlock(delayMs, repeat, timeredMessage);
	}
}

// ***********************************************************
// Timer interrupt
//
ISR(TIMER1_COMPA_vect)
{
	// Since the last timer interrupt exactly a counter limit amount of ticks has ellapsed
	// No atomic protection is needed as we are in an interrupt routine
	Timer_AgeTimerBlocks(OCR1A);
}



