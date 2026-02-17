#ifndef ENGINE_SIM_H_
#define ENGINE_SIM_H_

#include <stdint.h>

void Engine_Init(void);
void Engine_Update(uint8_t throttle_percent);
uint16_t Engine_GetRPM(void);

#endif
