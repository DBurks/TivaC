//*****************************************************************************
//
// TivaC_UART_Project.c - A UART Project demonstration
//
// Modifes a simple helo world application to demonstrate different UART 
// formatting techniques.
//
// First we'll go through printing out a simulated temperature periodically 
// to the Serial Console. This will produce an endless stream of text.
//
// NJesxt use ANSI formatting, to produce a dash board showing a header 
// and the temperature. The temperature will be colored properly 
// depending upon whether it is above the max, between max and min ro below
// the min threshold value.
//
// Finally output json or csv over UART so the user can format the data,
// perhaps using an external application/
//
//*****************************************************************************

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>

#include "inc/hw_memmap.h"
#include "inc/hw_ints.h"
#include "inc/hw_types.h"
#include "driverlib/debug.h"
#include "driverlib/fpu.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
#include "utils/uartstdio.h"
#include "driverlib/i2c.h"
#include "inc/hw_i2c.h"

// ANSI Escpae Codes
#define ANSI_COLOR_RED    "\x1b[31m"   // color if temp above max
#define ANSI_COLOR_GREEN  "\x1b[32m"   // color if between max and min
#define ANSI_COLOR_BLUE   "\x1b[34m"   // color if temp below min
#define ANSI_COLOR_RESET  "\x1b[0m"    // reset color (black)
#define ANSI_CLEAR_SCREEN "\x1b[2J"    // ansi control characters to clear screen
#define ANSI_CURSOR_HOME  "\x1b[H"     // returns the cursor to the home position

// Simulation Constraints
#define TEMP_MID         25.0
#define TEMP_AMP         10.0

// Sensor Address
#define AHT20_ADDR 0x38

// add modes between human readable, machine-json and machine-csv
typedef enum {
    MODE_HUMAN,         // ANSI, escape characters
    MODE_MACHINE_JSON,  // machine readable json
    MODE_MACHINE_CSV    // machine readable csv format
} OutputMode_t;


typedef enum {
    ALARM_NONE = 0, 
    ALARM_LOW = 1,
    ALARM_HIGH = 2
} AlarmState_t;

// Thresholds are now variables, not constants
volatile float g_ui32TempMax = 30.0;
volatile float g_ui32TempMin = 20.0;

// Buffer for incoming characters
char g_cInputBuffer[10];
int g_iBufferIdx = 0;

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1> TivaC Series UART Demonstration</h1>
//!
//! UART0, connected to the Virtual Serial Port and running at
//! 115,200, 8-N-1, is used to display messages from this application.
//
//*****************************************************************************

//*****************************************************************************
//
// The error routine that is called if the driver library encounters an error.
//
//*****************************************************************************
#ifdef DEBUG
void
__error__(char *pcFilename, uint32_t ui32Line)
{
}
#endif


void UARTIntHandler(void) {
    uint32_t ui32Status;
    ui32Status = ROM_UARTIntStatus(UART0_BASE, true); // Get interrupt status
    ROM_UARTIntClear(UART0_BASE, ui32Status);        // Clear the interrupt

    while(ROM_UARTCharsAvail(UART0_BASE)) {
    char c = ROM_UARTCharGetNonBlocking(UART0_BASE);

        // If we hit a newline, process the command
        if (c == '\n' || c == '\r') {
            g_cInputBuffer[g_iBufferIdx] = '\0'; // Null terminate
            
            if (g_iBufferIdx > 1) {
                // Parse: M35 or L15
                char cmd = g_cInputBuffer[0];
                float val = (float)atoi(&g_cInputBuffer[1]);

                if (cmd == 'M') g_ui32TempMax = val;
                if (cmd == 'L') g_ui32TempMin = val;
            }
            g_iBufferIdx = 0; // Reset buffer
        } 
        else if (g_iBufferIdx < 9) {
            g_cInputBuffer[g_iBufferIdx++] = c;
        }
    }
}


//*****************************************************************************
//
// Configure the UART and its pins.  This must be called before UARTprintf().
//
//*****************************************************************************
void
ConfigureUART(void)
{
    //
    // Enable the GPIO Peripheral used by the UART.
    //
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

    //
    // Enable UART0
    //
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);

    //
    // Configure GPIO Pins for UART mode.
    //
    MAP_GPIOPinConfigure(GPIO_PA0_U0RX);
    MAP_GPIOPinConfigure(GPIO_PA1_U0TX);
    MAP_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    //
    // Use the internal 16MHz oscillator as the UART clock source.
    //
    UARTClockSourceSet(UART0_BASE, UART_CLOCK_PIOSC);

    //
    // Initialize the UART for console I/O.
    //
    UARTStdioConfig(0, 115200, 16000000);

    // Enable UART Interrupts
    ROM_IntEnable(INT_UART0); 
    ROM_UARTIntEnable(UART0_BASE, UART_INT_RX | UART_INT_RT);
    
    // Set the handler (if not defined in startup_ccs.c)
    UARTIntRegister(UART0_BASE, UARTIntHandler);

}

void ConfigureI2C(void) {
    // Enable I2C1 and GPIOA peripherals
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOA));
    
    SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C1);
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_I2C1));
    
    // Configure pins for I2C1 (PA6 = SCL and PA7 = SDA)
    GPIOPinConfigure(GPIO_PA6_I2C1SCL);
    GPIOPinConfigure(GPIO_PA7_I2C1SDA);
    
    GPIOPinTypeI2CSCL(GPIO_PORTA_BASE, GPIO_PIN_6);
    GPIOPinTypeI2C(GPIO_PORTA_BASE, GPIO_PIN_7);

    // Initialize Master (false = 100knps, true = 400 kbps)
    I2CMasterInitExpClk(I2C1_BASE, SysCtlClockGet(), false);
}

void ReadAHT20(float *temp, float *hum) {
    uint32_t data[6];
    
    // 1. Send Trigger Measurement Command (0xAC, 0x33, 0x00)
    I2CMasterSlaveAddrSet(I2C1_BASE, AHT20_ADDR, false);
    I2CMasterDataPut(I2C1_BASE, 0xAC);
    I2CMasterControl(I2C1_BASE, I2C_MASTER_CMD_BURST_SEND_START);
    while(I2CMasterBusy(I2C1_BASE));

    I2CMasterDataPut(I2C1_BASE, 0x33);
    I2CMasterControl(I2C1_BASE, I2C_MASTER_CMD_BURST_SEND_CONT);
    while(I2CMasterBusy(I2C1_BASE));

    I2CMasterDataPut(I2C1_BASE, 0x00);
    I2CMasterControl(I2C1_BASE, I2C_MASTER_CMD_BURST_SEND_FINISH);
    while(I2CMasterBusy(I2C1_BASE));

    // 2. Wait ~80ms for sensor to finish
    SysCtlDelay(SysCtlClockGet() / (30 * 12));

    // 2b. Poll busy bit
    uint8_t status = 0x80;
    I2CMasterSlaveAddrSet(I2C1_BASE, AHT20_ADDR, true);

    while (status & 0x80) {
        I2CMasterControl(I2C1_BASE, I2C_MASTER_CMD_SINGLE_RECEIVE);
        while (I2CMasterBusy(I2C1_BASE));
        status = I2CMasterDataGet(I2C1_BASE);
    }



    // 3. Read 6 bytes back
    I2CMasterSlaveAddrSet(I2C1_BASE, AHT20_ADDR, true);
    int i;
    for (i = 0; i < 6; i++) {
        uint32_t cmd = (i == 0) ? I2C_MASTER_CMD_BURST_RECEIVE_START :
                       (i == 5) ? I2C_MASTER_CMD_BURST_RECEIVE_FINISH :
                                  I2C_MASTER_CMD_BURST_RECEIVE_CONT;
        I2CMasterControl(I2C1_BASE, cmd);
        while (I2CMasterBusy(I2C1_BASE)) ;
        
        data[i] = I2CMasterDataGet(I2C1_BASE);
    }

    // 4. Conversion Math 
    uint32_t raw_hum = ((data[1] << 12) | (data[2] << 4) | (data[3] >> 4));
    uint32_t raw_temp = (((data[3] & 0x0F) << 16) | (data[4] << 8) | data[5]);

    *hum = (float) raw_hum * 100.0/ 1048576.0;
    *temp = ((float) raw_temp * 200.0 / 1045876.0) - 50.0;
}

void
DisplayTemp(uint32_t timestamp, float current_temp, float current_humidity, OutputMode_t currentMode ) {

    // compute the color to use
    char* color = ANSI_COLOR_GREEN;
    AlarmState_t alarm = ALARM_NONE;
    
    if (current_temp >= g_ui32TempMax) {
        color = ANSI_COLOR_RED;
        alarm = ALARM_HIGH;
    }
    else if (current_temp <= g_ui32TempMin) {
        color = ANSI_COLOR_BLUE;
        alarm = ALARM_LOW;
    }
        
    if (currentMode == MODE_HUMAN) {
        
        // use home to overwrite dashboard in place
        UARTprintf(ANSI_CURSOR_HOME);
        UARTprintf("----------------------------------\n");
        UARTprintf("HTIVA C LIVE TELEMETRY            \n");
        UARTprintf("----------------------------------\n");
        UARTprintf(" Temp: %s%d.%d C%s \n", color, (int) current_temp, (int)(current_temp *10) %10, ANSI_COLOR_RESET);
        UARTprintf("----------------------------------\n");
    } else if (currentMode == MODE_MACHINE_JSON) {
        UARTprintf("{\"time\": %d, \"temp\": %d.%d, \"humidity\":%d.%d, \"alarm\": %d, \"source\":\"AHT20\" }\n", 
            timestamp, (int)current_temp, (int) (current_temp * 10) % 10,
             (int)current_humidity, (int) (current_humidity * 10) % 10,
            alarm
        );
    }
    else if (currentMode == MODE_MACHINE_CSV) {
        UARTprintf("%d, %d.%d, %d\n", 
            timestamp, (int)current_temp, (int) (current_temp * 10) % 10,
            alarm
        );
    }
}


//*****************************************************************************
//
// Print text with the Temperature data to the UART
//
//*****************************************************************************
int
main(void)
{

    //
    // Enable lazy stacking for interrupt handlers.  This allows floating-point
    // instructions to be used within interrupt handlers, but at the expense of
    // extra stack usage.
    //
    MAP_FPULazyStackingEnable();

    //
    // Set the clocking to run directly from the crystal.
    //
    MAP_SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_XTAL_16MHZ |
                       SYSCTL_OSC_MAIN);

    //
    // Initialize the UART.
    //
    ConfigureUART();

    ConfigureI2C();

    // we need an angle to feed into the sine functino
    float angle = 0.0;

    // set my timestamp ticker
    uint32_t timestamp = 0;

    // initialize the output mode (default to json)
    OutputMode_t currentMode = MODE_MACHINE_JSON;

    //
    // We are finished.  Hang around doing nothing.
    //
    while(1)
    {
        // calculate our simulated temperature
        float current_temp, current_humidity;
        ReadAHT20(&current_temp, &current_humidity);

        // display the temperature data
        DisplayTemp(timestamp, current_temp, current_humidity, currentMode);
        
        // change our angle evry cycle and increment timestamp
        angle += 0.05;
        timestamp++;
        
        // wait a little bit
        SysCtlDelay(SysCtlClockGet() / 10);
    }
}
