#ifndef MAIN
#define MAIN

void MonitorAllButtons(struct QueueElementT* message);
void ButtonPressed(struct QueueElementT* message);
void ButtonReleased(struct QueueElementT* message);
void LedSwitch(struct QueueElementT* message);

#endif
