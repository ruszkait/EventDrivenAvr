#include <util\atomic.h>
#include <stdbool.h>
#include "heap.h"

struct Heap_MemoryBlockHeaderT
{
	Heap_MemoryBlockSizeT	m_HMBContentSize;
	int8_t					m_PinCounter;
};

typedef struct Heap_MemoryBlockHeaderT* RedirectionT;

struct Heap_CtrlAreaT
{
	// Redirection area
	RedirectionT*					m_redirectionAreaBegin;
	RedirectionT*					m_redirectionAreaEnd;

	// Start HMB of the compaction
	// This points to the first unused HMB, this will be starting
	// point of the next compaction
	struct Heap_MemoryBlockHeaderT*	m_nextCompactStart;

	// The source/destination heap block of the currently 
	// running compaction
	struct Heap_MemoryBlockHeaderT*	m_currentCompactSource;
	struct Heap_MemoryBlockHeaderT*	m_currentCompactDestination;

	// Start of the free heap
	void*							m_freeBegin;
};


// ***********************************************************
// 
//
void Heap_Init(void* heapMemory, int16_t sizeInBytes)
{
	struct Heap_CtrlAreaT* controlArea = (struct Heap_CtrlAreaT*)heapMemory;
	
	controlArea->m_freeBegin = (struct Heap_MemoryBlockHeaderT*)((char*)heapMemory + sizeof(struct Heap_CtrlAreaT));
	controlArea->m_redirectionAreaBegin = ((RedirectionT*)((char*)heapMemory + sizeInBytes)) - 1;
	// Begin = End -> no handle available
	controlArea->m_redirectionAreaEnd = controlArea->m_redirectionAreaBegin;

	// No compaction runs now
	controlArea->m_currentCompactSource = 0;
	controlArea->m_currentCompactDestination = 0;

	// No next compaction
	controlArea->m_nextCompactStart = 0;
}

// ***********************************************************
// 
//
Heap_MemoryBlockSizeT Heap_FreeMemorySize(void* heapMemory)
{
	struct Heap_CtrlAreaT* controlArea = (struct Heap_CtrlAreaT*)heapMemory;
	// Full available memory would be m_heapRedirectionAreaEnd - m_freeBegin + 2 (because the m_heapRedirectionAreaEnd points to an unused redirection)
	// But if we want to allocate the memory then we create a redirection anyway, so that extra two bytes are not available for data content
	return (char*)controlArea->m_redirectionAreaEnd - (char*)controlArea->m_freeBegin;
}

// ***********************************************************
// 
//
Heap_RedirectionIndexT Heap_Alloc(void* heapMemory, Heap_MemoryBlockSizeT sizeInBytes)
{
	if (sizeInBytes > Heap_FreeMemorySize(heapMemory))
		return 0;
	
	struct Heap_CtrlAreaT* controlArea = (struct Heap_CtrlAreaT*)heapMemory;
	//
	// Create Redirection to the new HMB
	//
	
	// Find an unused redirection
	Heap_RedirectionIndexT redirectionIndex = 0;
	RedirectionT* redirectionIter = controlArea->m_redirectionAreaBegin;
	for(RedirectionT* iterEnd = controlArea->m_redirectionAreaEnd; redirectionIter != iterEnd; --redirectionIter)
	{
		// Break loop if the current redirection is a NULL, so it is unused
		if (!(*redirectionIter))
			break;

		++redirectionIndex;
	}

	// If there was no reusable redirection, then we used the End redirection
	// So the end must step further
	if (redirectionIter == controlArea->m_redirectionAreaEnd)
	{
		--controlArea->m_redirectionAreaEnd;
	}

	// Let the new redirection point to the beginning of the new HMB
	*redirectionIter = (struct Heap_MemoryBlockHeaderT*)controlArea->m_freeBegin;

	//
	// Initialize the new HMB
	//

	// Set Heap Memory Block header
	struct Heap_MemoryBlockHeaderT* newHMB = *redirectionIter;
	newHMB->m_HMBContentSize = sizeInBytes;
	newHMB->m_PinCounter = 0;

	// Move free to the end of the new HMB
	controlArea->m_freeBegin = (char*)controlArea->m_freeBegin + sizeof(struct Heap_MemoryBlockHeaderT)/*HMB Header size*/ + sizeInBytes/*HMB Content*/;

	// Return the new redirection index
	return redirectionIndex;
}

// ***********************************************************
// 
//
void Heap_Free(void* heapMemory, Heap_RedirectionIndexT redirectionIndex)
{
	struct Heap_CtrlAreaT* controlArea = (struct Heap_CtrlAreaT*)heapMemory;

	// Find the HMB where the redirection points to
	RedirectionT* redirection = controlArea->m_redirectionAreaBegin - redirectionIndex;

	struct Heap_MemoryBlockHeaderT* freedHeapBlock = *redirection;

	// Because of the freed heap block we will need compaction
	if (0 != controlArea->m_nextCompactStart)
	{
		// The next compaction start shall point to the very first
		// unused heap block
		if (controlArea->m_nextCompactStart > freedHeapBlock)
		{
			controlArea->m_nextCompactStart = freedHeapBlock;
		}
	}
	else
	{
		// This heap block is the first freed one
		controlArea->m_nextCompactStart = freedHeapBlock;
	}

	// invalidate the redirection
	*redirection = 0;
}

// ***********************************************************
// 
//
void* Heap_Pin(void* heapMemory, Heap_RedirectionIndexT redirectionIndex)
{
	struct Heap_CtrlAreaT* controlArea = (struct Heap_CtrlAreaT*)heapMemory;

	// Find the HMB where the redirection points to
	RedirectionT* redirection = controlArea->m_redirectionAreaBegin - redirectionIndex;
	struct Heap_MemoryBlockHeaderT* hmb = *redirection;

	void* heapContent = (void*)((char*)hmb + sizeof(struct Heap_MemoryBlockHeaderT));

	// Disable interrupts within this block
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		++(hmb->m_PinCounter);
	}

	return heapContent;
}

// ***********************************************************
// 
//
void Heap_UnPin(void* heapMemory, Heap_RedirectionIndexT redirectionIndex)
{
	struct Heap_CtrlAreaT* controlArea = (struct Heap_CtrlAreaT*)heapMemory;

	// Find the HMB where the redirection points to
	RedirectionT* redirection = controlArea->m_redirectionAreaBegin - redirectionIndex;
	struct Heap_MemoryBlockHeaderT* hmb = *redirection;

	// Disable interrupts within this block
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		--(hmb->m_PinCounter);
	}
}

// ***********************************************************
//
//
bool Heap_CompactOneBlock(void* heapMemory)
{
	struct Heap_CtrlAreaT* controlArea = (struct Heap_CtrlAreaT*)heapMemory;
	char *source = (char*)controlArea->m_currentCompactSource;
	char *destination = (char*)controlArea->m_currentCompactDestination;

	struct Heap_MemoryBlockHeaderT* sourceHMB = (struct Heap_MemoryBlockHeaderT*)source;

	// Find the first redirection that points to this HMB
	RedirectionT* firstRedirectionToSourceHMB = controlArea->m_redirectionAreaBegin;
	RedirectionT* redirectionEnd = controlArea->m_redirectionAreaEnd;
	for(; firstRedirectionToSourceHMB != redirectionEnd; --firstRedirectionToSourceHMB)
	{
		if (sourceHMB == *firstRedirectionToSourceHMB)
			break;
	}

	Heap_MemoryBlockSizeT sourceHMBSize = sizeof(struct Heap_MemoryBlockHeaderT) + sourceHMB->m_HMBContentSize;

	bool blockWasPinned = false;

	// Moving each heap block must be atomic
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		// if the heap block is pinned then abort the compacting
		if (0 != sourceHMB->m_PinCounter)
		{
			blockWasPinned = true;

			// exit from the atomic block
			break;
		}

		// No redirection to the current heap block -> unused heap block
		if (redirectionEnd == firstRedirectionToSourceHMB)
		{
			// Skip this HMB from copying and step to the next HMB
			source += sourceHMBSize;

			// exit from the atomic block
			break;
	  	}

		// Update all redirections that point to this heap block
		RedirectionT* redirectionToUpdate = firstRedirectionToSourceHMB;
		for(; redirectionToUpdate != redirectionEnd; --redirectionToUpdate)
		{
			if (sourceHMB == *redirectionToUpdate)
			{
				*redirectionToUpdate = (struct Heap_MemoryBlockHeaderT*)destination;
			}
		}

		// Copy the HMB at source to the destination
		for (Heap_MemoryBlockSizeT bytesToCopy = sourceHMBSize; 0 != bytesToCopy; --bytesToCopy)
		{
			*destination = *source;
			++destination;
			++source;
		}
	}

	if (!blockWasPinned)
	{
		controlArea->m_currentCompactSource = (struct Heap_MemoryBlockHeaderT*)source;
		controlArea->m_currentCompactDestination = (struct Heap_MemoryBlockHeaderT*)destination;
	}

	return !blockWasPinned;
}

// ***********************************************************
//
//
void Heap_CompactAllBlocks(void* heapMemory)
{
	struct Heap_CtrlAreaT* controlArea = (struct Heap_CtrlAreaT*)heapMemory;

	// If there is a compaction already running then finish that
	if (0 != controlArea->m_currentCompactSource)
	{
		while (controlArea->m_currentCompactSource != controlArea->m_freeBegin)
		{
			// If the compaction of a block fails
			// (an interrupt pins the current heap block)
			// then abort the curren compaction.
			// Later we can continue from that point.
			if (!Heap_CompactOneBlock(heapMemory))
			{
				return;
			}
		}

		// Compaction done

		// The new free area start at the end of the compacted area
		controlArea->m_freeBegin = controlArea->m_currentCompactDestination;

		// Current compaction finished
		controlArea->m_currentCompactSource = 0;
		controlArea->m_currentCompactDestination = 0;

		// Compact the redirection list
		// Remove the invalid redirections from the end of the list
		RedirectionT* redirectionIter = controlArea->m_redirectionAreaEnd - 1;
		RedirectionT* redirectionStart = controlArea->m_redirectionAreaBegin;
		for(; redirectionIter <= redirectionStart; ++redirectionIter)
		{
			if (0 != *redirectionIter)
				break;
		}
		controlArea->m_redirectionAreaEnd = redirectionIter - 1;
	}
	else
	{
		// There was/were freed block(s), because the compaction start point
		// is set
		if (0 != controlArea->m_nextCompactStart)
		{
			// Setup the next compaction starting points
			controlArea->m_currentCompactSource = controlArea->m_nextCompactStart;
			controlArea->m_currentCompactDestination = controlArea->m_currentCompactSource;
			controlArea->m_nextCompactStart = 0;
		}
	}
}

/*
Heap layout:

**************************************************
*   Heap_CtrlAreaT::m_heapRedirectionAreaBegin   * ------------------------+   <- heap memory
*   Heap_CtrlAreaT::m_heapRedirectionAreaEnd     * -------------------+    +
*   Heap_CtrlAreaT::m_freeBegin                  * --------------+    +    +
**************************************************               +    +    +
*   Heap Memory Block #1 Header				     * <-----+       +    +    +
*   Heap Memory Block #1 Content                 *       +       +    +    +
*   Heap Memory Block #1 Content                 *       +       +    +    +
*   Heap Memory Block #1 Content                 *       +       +    +    +
**************************************************       +       +    +    +
*   Heap Memory Block #2 Header		   	         * <-+   +       +    +    +
*   Heap Memory Block #2 Content                 *   |   +       +    +    +
*   Heap Memory Block #2 Content                 *   |   +       +    +    +
*   Heap Memory Block #2 Content                 *   |   +       +    +    +
**************************************************   |   +       +    +    +
*                                                *   |   +  <----+    +    +
*                                                *   |   +            +    +
*                                                *   |   +            +    +
*              Free heap                         *   |   +            +    +
*                                                *   |   +            +    +
*                                                *   |   +            +    +
*                                                *   |   +            +    +
*                                                *   |   +   <--------+    +
**************************************************   |   +                 +
*  Redirection #3                                * --+   +                 +
**************************************************       +                 +
*  Redirection #2 = 0 (freed)                    *       +                 +
**************************************************       +                 +
*  Redirection #1                                * ------+   <-------------+
**************************************************


*/
