#include <stdint.h>
#include <stdbool.h>
#include "driverlib/sysctl.h"
#include "driverlib/interrupt.h"
#include "can_handler.h"
#include "uart_handler.h"

int main(void) {
    // 40MHz Clock
    SysCtlClockSet(SYSCTL_SYSDIV_5 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);

    UART_Init();
    CAN_Network_Init();
    
    IntMasterEnable();

    uint16_t rpm = 0;
    uint8_t ack = 0;
    uint8_t requestedThrottle = 0;

    while(1) {
        // PC -> CAN
        if(UART_GetCommand(&requestedThrottle)) {
            CAN_SendCommand(requestedThrottle);
        }

        // CAN -> PC
        if(CAN_GetTelemetry(&rpm, &ack)) {
            UART_SendJSON(rpm, ack);
        }

        // Small delay to keep the loop stable
        SysCtlDelay(SysCtlClockGet() / 3 / 1000); 
    }
}
