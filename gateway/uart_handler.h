#ifndef UART_HANDLER_H_
#define UART_HANDLER_H_

#include <stdint.h>
#include <stdbool.h>

// Initializes UART0 (PA0/PA1) at 115200 baud
void UART_Init(void);

// Formats data into JSON and sends to PC
void UART_SendJSON(uint16_t rpm, uint8_t throttle_ack);

// Checks for incoming string commands from PC. Returns true if valid 0-100.
bool UART_GetCommand(uint8_t *cmd);

#endif /* UART_HANDLER_H_ */
