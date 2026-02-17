#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h> // for atoi
#include "inc/hw_memmap.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
#include "utils/uartstdio.h"
#include "uart_handler.h"

void UART_Init(void) {
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);

    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    UARTStdioConfig(0, 115200, SysCtlClockGet());
}

void UART_SendJSON(uint16_t rpm, uint8_t throttle_ack) {
    // Escaping the double quotes for valid JSON
    UARTprintf("{\"rpm\": %d, \"throttle\": %d}\n", rpm, (int)throttle_ack);
}

bool UART_GetCommand(uint8_t *cmd) {
    if(UARTCharsAvail(UART0_BASE)) {
        char buf[16];
        UARTgets(buf, 16);
        
        int val = atoi(buf);
        if(val >= 0 && val <= 100) {
            *cmd = (uint8_t)val;
            return true;
        }
    }
    return false;
}
