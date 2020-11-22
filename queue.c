#include "queue.h"

void Queue_Init(struct QueueT *p_this, QueueDataT *p_Buffer, QueueIndexT p_BufferSize)
{
	p_this->m_Buffer = p_Buffer;
	p_this->m_BufferAddressMask = p_BufferSize - 1;
	p_this->m_WriteIndex = 0;
	p_this->m_ReadIndex = 0;
}

bool Queue_IsEmpty(struct QueueT *p_this)
{
	// If the read and write index are the same then the buffer is empty
	return (p_this->m_WriteIndex == p_this->m_ReadIndex);
}

bool Queue_Pop(struct QueueT *p_this, struct QueueElementT *p_Element)
{
	// If the read and write index are the same then the buffer is empty
	register QueueIndexT readIndex = p_this->m_ReadIndex;
	if (p_this->m_WriteIndex == readIndex)
	{
		// No data was popped
		return false;
	}

    // Get the size of the data part of the element and store it in the element structure
	register QueueIndexT dataSize =	*((QueueIndexT*)(p_this->m_Buffer + readIndex)) -
									sizeof(QueueIndexT) /*sizeof(data size)*/ -
									sizeof(QueueCommandT) /*sizeof(command)*/;
	p_Element->m_DataSize = dataSize;

	const QueueIndexT bufferAddressMask = p_this->m_BufferAddressMask;

	// Step to the command part
	readIndex = (readIndex + sizeof(QueueIndexT)) & bufferAddressMask;

	p_Element->m_Command = *(QueueCommandT*)(p_this->m_Buffer + readIndex);

	if (0 == dataSize)
	{
		// If the message has no data part then pop the message

		// Step to the data part
		readIndex = (readIndex + sizeof(QueueCommandT)) & bufferAddressMask;

		// Store the new read index in the queue control block
		p_this->m_ReadIndex = readIndex;
	}
	else
	{
		// If the message has data part, but no buffer to copy was
		// given then skip popping the message -> preview of the message

		if (0 != p_Element->m_Data)
		{
			// Step to the data part
			readIndex = (readIndex + sizeof(QueueCommandT)) & bufferAddressMask;

			// Copy the Queue data into the element data part	
			register QueueDataT *elementData = p_Element->m_Data;
			for (;0 != dataSize; --dataSize)
			{
				// Read that data where the read index points
				*elementData = p_this->m_Buffer[readIndex];

				// Step to the next byte
				readIndex = (readIndex + 1) & bufferAddressMask;

				// Step to the next element data cell		
				++elementData;
			}

			// Store the new read index in the queue control block
			p_this->m_ReadIndex = readIndex;
		}
	}

	return true;
}

bool Queue_Push(struct QueueT *p_this, struct QueueElementT *p_Element)
{
	// Check if with this data amount we would run on the read index. This would mean that the buffer is full.
	register QueueIndexT elementSize =	p_Element->m_DataSize +
										sizeof(QueueIndexT) + /*sizeof(element size parameter)*/
										sizeof(QueueCommandT); /*sizeof(command)*/;
	const QueueIndexT bufferAddressMask = p_this->m_BufferAddressMask;
	register QueueIndexT writeIndex = p_this->m_WriteIndex;
	if (((p_this->m_ReadIndex - writeIndex - 1) & bufferAddressMask) < elementSize)
	{
		// No data was pushed
		return false;
	}

	// Copy the queue the size first
	*(QueueIndexT*)(p_this->m_Buffer + writeIndex) = elementSize;

	// Step to the new position modulo the bitmask
	writeIndex = (writeIndex + sizeof(QueueIndexT)) & bufferAddressMask;

	// Copy the command code
	*(QueueCommandT*)(p_this->m_Buffer + writeIndex) = p_Element->m_Command;

	// Step to the new position modulo the bitmask
	writeIndex = (writeIndex + sizeof(QueueCommandT)) & bufferAddressMask;

	// Copy the data
	register QueueDataT *elementData = p_Element->m_Data;
	for (register QueueIndexT dataSize = p_Element->m_DataSize; 0 != dataSize; --dataSize)
	{
		// Write that data where the write index points
		p_this->m_Buffer[writeIndex] = *elementData;

		// Step to the new position modulo the bitmask
		writeIndex = (writeIndex + 1) & bufferAddressMask;
		
		// Step one on the element buffer, too
		++elementData;
	}

	// Store the new write index in the queue control block
	p_this->m_WriteIndex = writeIndex;
	
	// The element was pushed
	return true;
}



