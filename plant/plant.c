#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "driverlib/sysctl.h"
#include "driverlib/interrupt.h"
#include "engine_sim.h"
#include "can_handler.h"


int main(void) {
    // System Clock Init
    SysCtlClockSet(SYSCTL_SYSDIV_5 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);
    
    // Initialize abstracted modules
    Engine_Init();
    CAN_Network_Init();
    IntMasterEnable();

    while(1) {
        // 1. Get the latest command from the CAN driver
        uint8_t cmd = CAN_GetLastCommand();

        // 2. Update the physics engine
        Engine_Update(cmd);

        // 3. Send the results back out to the bus
        CAN_SendTelemetry(Engine_GetRPM(), cmd);

        // Loop delay (approx 20ms)
        SysCtlDelay(SysCtlClockGet() / 3 / 50);
    }
}
