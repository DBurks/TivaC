#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_can.h"
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "driverlib/can.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/sysctl.h"
#include "can_handler.h"

// Internal state
static volatile uint8_t g_ui8CurrentThrottleCmd = 0;
static tCANMsgObject sMsgObjRX;
static tCANMsgObject sMsgObjTX;
static uint8_t pui8MsgDataRX[8];
static uint8_t pui8MsgDataTX[8];

// --- Interrupt Handler ---
// This must be mapped in tm4c123gh6pm_startup_ccs.c
void CAN0IntHandler(void) {
    uint32_t ui32Status = CANIntStatus(CAN0_BASE, CAN_INT_STS_CAUSE);

    if(ui32Status == CAN_INT_INTID_STATUS) {
        ui32Status = CANStatusGet(CAN0_BASE, CAN_STS_CONTROL);
        if((ui32Status & CAN_STATUS_LEC_MSK) == CAN_STATUS_LEC_ACK) {
            GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, GPIO_PIN_1);
        } else {
            GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, 0);
        }
    }
    // --- TELEMETRY TX COMPLETED ---
    else if(ui32Status == 1) { 
        CANIntClear(CAN0_BASE, 1);
        // We don't need to do much here other than clear it
    }
    // --- COMMAND RX RECEIVED (The missing piece!) ---
    else if(ui32Status == 2) { 
        uint8_t pui8Data[8];
        tCANMsgObject sMsg;
        sMsg.pui8MsgData = pui8Data;
    
        // 1. CLEAR the interrupt immediately
        CANIntClear(CAN0_BASE, 2); 

        // 2. READ the message (this also helps clear hardware flags)
        CANMessageGet(CAN0_BASE, 2, &sMsg, true); 

        // 3. Update the variable
        g_ui8CurrentThrottleCmd = pui8Data[0]; 

        // 4. TOGGLE (Don't just turn ON) the LED
        // This way, you see a blink every time a command hits.
        uint32_t currentLED = GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_2);
        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, currentLED ^ GPIO_PIN_2);
    }
}

// --- Driver Logic ---
void CAN_Network_Init(void) {
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3);

    // Enable Peripherals
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_CAN0);

    // Configure Pins PB4/PB5
    GPIOPinConfigure(GPIO_PB4_CAN0RX);
    GPIOPinConfigure(GPIO_PB5_CAN0TX);
    GPIOPinTypeCAN(GPIO_PORTB_BASE, GPIO_PIN_4 | GPIO_PIN_5);

    CANInit(CAN0_BASE);
    
    // 500kbps is standard for these demos
    CANBitRateSet(CAN0_BASE, SysCtlClockGet(), 500000);

    // Enable Interrupts on the CAN peripheral
    // CORRECT: Master and Error interrupts are sufficient
    CANIntEnable(CAN0_BASE, CAN_INT_MASTER | CAN_INT_ERROR | CAN_INT_STATUS);
    IntEnable(INT_CAN0);

    // Set up RX Object (ID: 0x200 - Commands from Controller)
    sMsgObjRX.ui32MsgID = 0x200;
    sMsgObjRX.ui32MsgIDMask = 0x7FF;
    sMsgObjRX.ui32Flags = MSG_OBJ_RX_INT_ENABLE | MSG_OBJ_USE_ID_FILTER;
    sMsgObjRX.ui32MsgLen = 8;
    sMsgObjRX.pui8MsgData = pui8MsgDataRX;
    CANMessageSet(CAN0_BASE, 2, &sMsgObjRX, MSG_OBJ_TYPE_RX);

    CANEnable(CAN0_BASE);
}

void CAN_SendTelemetry(uint16_t rpm, uint8_t throttle) {
    pui8MsgDataTX[0] = (uint8_t)(rpm >> 8);   // High Byte
    pui8MsgDataTX[1] = (uint8_t)(rpm & 0xFF); // Low Byte
    pui8MsgDataTX[2] = throttle;

    sMsgObjTX.ui32MsgID = 0x101; // Telemetry ID
    sMsgObjTX.ui32MsgLen = 3;
    sMsgObjTX.pui8MsgData = pui8MsgDataTX;

    sMsgObjTX.ui32Flags = MSG_OBJ_TX_INT_ENABLE;

    // Use Object 2 for TX to avoid colliding with RX Object 1
    CANMessageSet(CAN0_BASE, 1, &sMsgObjTX, MSG_OBJ_TYPE_TX);
}

uint8_t CAN_GetLastCommand(void) {
    return g_ui8CurrentThrottleCmd;
}
