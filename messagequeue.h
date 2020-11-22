#ifndef MESSAGEQUEUE
#define MESSAGEQUEUE

#include <stdbool.h>
#include "queue.h"

//
// Message Queue API
//

// Initiliazes the message queue
void MessageQueue_Init(void);

// Pop one element fom the queue
// ISR safe, can be called from ISR, too
// Parameters: The p_Element must have a valid buffer pointer
// Returns: if message was get from the queue
bool MessageQueue_Pop(struct QueueElementT *p_Element);

// Push one element into the queue
// ISR safe, can be called from ISR, too
// Warning: Works wrong if the data to be pushed in is equal or larger than the size of the ring buffer.
// Returns: if message was put in the queue
bool MessageQueue_Push(struct QueueElementT *p_Element);

// Returns if the queue is empty
// ISR safe, can be called from ISR, too
bool MessageQueue_IsEmpty();

#endif

