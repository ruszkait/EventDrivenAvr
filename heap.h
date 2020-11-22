#ifndef HEAP
#define HEAP

#include "config.h"

#include <stdint.h>

typedef uint8_t		Heap_RedirectionIndexT;
typedef uint16_t 	Heap_MemoryBlockSizeT;

//
// Heap API
//

// Initializes a heap memory
void Heap_Init(void* heapMemory, int16_t sizeInBytes);

// Creates a heap block. Initally the block is floating
Heap_RedirectionIndexT Heap_Alloc(void* heapMemory, Heap_MemoryBlockSizeT sizeInBytes);

// Pins the floating heap block
// ISR safe, can be called from ISR, too
void* Heap_Pin(void* heapMemory, Heap_RedirectionIndexT redirectionIndex);

// UnPins the heap block, so it will be floating again
// ISR safe, can be called from ISR, too
void Heap_UnPin(void* heapMemory, Heap_RedirectionIndexT redirectionIndex);

// Frees a heap block
void Heap_Free(void* heapMemory, Heap_RedirectionIndexT redirectionIndex);

// Recycles the gaps between the floating heap blocks
void Heap_CompactAllBlocks(void* heapMemory);

// Tells the size of the free space area
Heap_MemoryBlockSizeT Heap_FreeMemorySize(void* heapMemory);

#endif
