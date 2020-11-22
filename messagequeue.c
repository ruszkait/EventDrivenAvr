#include <util\atomic.h>
#include "messagequeue.h"
#include "config.h"

// Message queue control block
struct QueueT oMessageQueue;

// Message queue data block
QueueDataT oMessageQueueBuffer[MAIN_MESSAGE_QUEUE_SIZE];

void MessageQueue_Init(void)
{
	// Initiliaze the message queue
	Queue_Init(&oMessageQueue, oMessageQueueBuffer, sizeof(oMessageQueueBuffer)/sizeof(QueueDataT));
}

bool MessageQueue_Pop(struct QueueElementT *p_Element)
{
	bool result;
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		result = Queue_Pop(&oMessageQueue, p_Element);
	}
	return result;
}

bool MessageQueue_Push(struct QueueElementT *p_Element)
{
	bool result;
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		result = Queue_Push(&oMessageQueue, p_Element);
	}
	return result;
}

bool MessageQueue_IsEmpty()
{
	bool result;
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		result = Queue_IsEmpty(&oMessageQueue);
	}
	return result;
}














