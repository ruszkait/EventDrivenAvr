#ifndef QUEUE
#define QUEUE

#include <stdint.h>
#include <stdbool.h>

//
// Queue types
//

struct QueueElementT;

typedef void (*MessageProcessorT) (struct QueueElementT*);
#define QueueIndexT uint8_t
#define QueueDataT uint8_t
#define QueueCommandT MessageProcessorT


struct QueueT
{
	QueueDataT *m_Buffer;
	QueueIndexT m_BufferAddressMask;
	QueueIndexT m_WriteIndex;
	QueueIndexT m_ReadIndex;
};

struct QueueElementT
{
	QueueCommandT m_Command;
	QueueIndexT m_DataSize;
	QueueDataT *m_Data;
};

//
// General Queue API
//

// Initiliazes the queue
// Parameters: p_BufferSize must be a power of 2
void Queue_Init(struct QueueT *p_this, QueueDataT *p_Buffer, QueueIndexT p_BufferSize);

// Pop one element fom the queue
// Parameters: The p_Element must have a valid buffer pointer
// Returns: if message was get from the queue
bool Queue_Pop(struct QueueT *p_this, struct QueueElementT *p_Element);

// Push one element into the queue
// Warning: Works wrong if the data to be pushed in is equal or larger than the size of the ring buffer.
// Returns: if message was put in the queue
bool Queue_Push(struct QueueT *p_this, struct QueueElementT *p_Element);

// Returns if the queue is empty
bool Queue_IsEmpty(struct QueueT *p_this);

#endif


