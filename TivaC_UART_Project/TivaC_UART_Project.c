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
#include <stdbool.h>
#include "inc/hw_memmap.h"
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
#include <math.h>

// ANSI Escpae Codes
#define ANSI_COLOR_RED    "\x1b[31m"   // color if temp above max
#define ANSI_COLOR_GREEN  "\x1b[32m"   // color if between max and min
#define ANSI_COLOR_BLUE   "\x1b[34m"   // color if temp below min
#define ANSI_COLOR_RESET  "\x1b[0m"    // reset color (black)
#define ANSI_CLEAR_SCREEN "\x1b[2J"    // ansi control characters to clear screen
#define ANSI_CURSOR_HOME  "\x1b[H"     // returns the cursor to the home position

// Simulation Constraints
#define TEMP_MIN_ALARM   20.0
#define TEMP_MAX_ALARM   30.0
#define TEMP_MID         25.0
#define TEMP_AMP         10.0


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
}

void
DisplayTemp(float current_temp) {
    
    // compute the color to use
    char* color = ANSI_COLOR_GREEN;
    if (current_temp >= TEMP_MAX_ALARM) color = ANSI_COLOR_RED;
    else if (current_temp <= TEMP_MIN_ALARM) color = ANSI_COLOR_BLUE;
        
    // use home to overwrite dashboard in place
    UARTprintf(ANSI_CURSOR_HOME);
    UARTprintf("----------------------------------\n");
    UARTprintf("HTIVA C LIVE TELEMETRY            \n");
    UARTprintf("----------------------------------\n");
    UARTprintf(" Temp: %s%d.%d C%s \n", color, (int) current_temp, (int)(current_temp *10) %10, ANSI_COLOR_RESET);
    UARTprintf("----------------------------------\n");
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

    //
    // We are finished.  Hang around doing nothing.
    //
    while(1)
    {
        // calculate our simulated temperature
        float current_temp = TEMP_MID + TEMP_AMP * sinf(angle);

        // display the temperature data
        DisplayTemp(current_temp);

        // change our angle evry cycle
        angle += 0.05;
        
        // wait a little bit
        SysCtlDelay(SysCtlClockGet() / 10);
    }
}
