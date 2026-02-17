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

static volatile uint16_t g_ui16LatestRPM = 0;
static volatile uint8_t g_ui8LatestAck = 0;
static volatile bool g_bNewDataAvailable = false;

// The Hardware Interrupt Handler
void CAN0IntHandler(void) {
    uint32_t ui32Status = CANIntStatus(CAN0_BASE, CAN_INT_STS_CAUSE);

    if(ui32Status == CAN_INT_INTID_STATUS) {
        ui32Status = CANStatusGet(CAN0_BASE, CAN_STS_CONTROL);
    }
    else if(ui32Status == 1) { // Mailbox 1: Telemetry RX (ID 0x101)
        uint8_t pui8Data[8];
        tCANMsgObject sMsg;
        sMsg.pui8MsgData = pui8Data;
        
        CANIntClear(CAN0_BASE, 1);
        CANMessageGet(CAN0_BASE, 1, &sMsg, true);
        
        // Unpack Big Endian (High byte first)
        g_ui16LatestRPM = (pui8Data[0] << 8) | pui8Data[1];
        g_ui8LatestAck = pui8Data[2];
        g_bNewDataAvailable = true;
    } // Inside Gateway CAN0IntHandler
    else if(ui32Status == 2) { 
        // Mailbox 2 (Command TX) finished successfully
        CANIntClear(CAN0_BASE, 2); 
    }
}

void CAN_Network_Init(void) {
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_CAN0);

    GPIOPinConfigure(GPIO_PB4_CAN0RX);
    GPIOPinConfigure(GPIO_PB5_CAN0TX);
    GPIOPinTypeCAN(GPIO_PORTB_BASE, GPIO_PIN_4 | GPIO_PIN_5);

    CANInit(CAN0_BASE);
    CANBitRateSet(CAN0_BASE, SysCtlClockGet(), 500000);

    CANIntEnable(CAN0_BASE, CAN_INT_MASTER | CAN_INT_ERROR | CAN_INT_STATUS);
    IntEnable(INT_CAN0);

    // RX Object for Telemetry from Plant
    tCANMsgObject sMsgRX;
    sMsgRX.ui32MsgID = 0x101;
    sMsgRX.ui32MsgIDMask = 0x7FF;
    sMsgRX.ui32Flags = MSG_OBJ_RX_INT_ENABLE | MSG_OBJ_USE_ID_FILTER;
    sMsgRX.ui32MsgLen = 8;
    CANMessageSet(CAN0_BASE, 1, &sMsgRX, MSG_OBJ_TYPE_RX);

    CANEnable(CAN0_BASE);
}

void CAN_SendCommand(uint8_t throttle) {
    uint8_t data[1];
    data[0] = throttle;
    
    tCANMsgObject sMsgTX;
    sMsgTX.ui32MsgID = 0x200;
    sMsgTX.ui32MsgLen = 1;
    sMsgTX.ui32Flags = MSG_OBJ_TX_INT_ENABLE; // ADD THIS FLAG
    sMsgTX.pui8MsgData = data;
    
    CANMessageSet(CAN0_BASE, 2, &sMsgTX, MSG_OBJ_TYPE_TX);
}

bool CAN_GetTelemetry(uint16_t *rpm, uint8_t *ack) {
    if(!g_bNewDataAvailable) return false;
    
    *rpm = g_ui16LatestRPM;
    *ack = g_ui8LatestAck;
    g_bNewDataAvailable = false;
    return true;
}
