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

void
DisplayTemp(uint32_t timestamp, float current_temp, OutputMode_t currentMode ) {

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
        UARTprintf("{\"time\": %d, \"temp\": %d.%d, \"alarm\": %d}\n", 
            timestamp, (int)current_temp, (int) (current_temp * 10) % 10,
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
        float current_temp = TEMP_MID + TEMP_AMP * sinf(angle);

        // display the temperature data
        DisplayTemp(timestamp, current_temp, currentMode);
        
        // change our angle evry cycle and increment timestamp
        angle += 0.05;
        timestamp++;
        
        // wait a little bit
        SysCtlDelay(SysCtlClockGet() / 10);
    }
}
