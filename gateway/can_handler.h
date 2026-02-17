#ifndef CAN_HANDLER_H_
#define CAN_HANDLER_H_

#include <stdint.h>
#include <stdbool.h>

// Initialize CAN0 on Port B (PB4/PB5) at 500kbps
void CAN_Network_Init(void);

// Sends a throttle command (0-100) to ID 0x200
void CAN_SendCommand(uint8_t throttle_percent);

// Checks if new telemetry has arrived from the Plant. 
// If true, updates rpm and ack pointers.
bool CAN_GetTelemetry(uint16_t *rpm, uint8_t *ack);

#endif /* CAN_HANDLER_H_ */
