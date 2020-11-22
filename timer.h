#include <stdint.h>
#include "queue.h"

//
// Timer API
//

#define TIMER_INFINITE_REPEAT 255

// Initiliazes the timer framework
void Timer_Init(void);

// Add a new timer to the timer framework
void Timer_Add(int32_t delayMs, uint8_t repeat, struct QueueElementT *timeredMessage);



