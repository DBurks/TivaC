#ifndef CAN_HANDLER_H_
#define CAN_HANDLER_H_

#include <stdint.h>

void CAN_Network_Init(void);
void CAN_SendTelemetry(uint16_t rpm, uint8_t throttle);
uint8_t CAN_GetLastCommand(void);

#endif
