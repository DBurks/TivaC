
#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_i2c.h"
#include "driverlib/i2c.h"
#include "driverlib/debug.h"
#include "driverlib/fpu.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
#include "utils/uartstdio.h"

#define EEPROM_1    0x50
#define EEPROM_2    0x51
#define BUF_SIZE    256  // 128 entries per chip * 2 chips
#define ENTRIES_PER_CHIP 128

/**
 * some variables for placing data in the ring buffer.
 */
typedef struct {
    uint8_t temp;
    uint8_t humidity;
} SensorData;

uint16_t write_ptr = 0;
uint16_t read_ptr = 0;

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

void InitI2C0(void) {
    // Enable I2C0 and GPIOB peripherals
    SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);

    // Configure the pin muxing for I2C0 functions on port B2 and B3.
    GPIOPinConfigure(GPIO_PB2_I2C0SCL);
    GPIOPinConfigure(GPIO_PB3_I2C0SDA);

    // Select the I2C function for these pins.
    GPIOPinTypeI2CSCL(GPIO_PORTB_BASE, GPIO_PIN_2);
    GPIOPinTypeI2C(GPIO_PORTB_BASE, GPIO_PIN_3);

    // Initialize the I2C Master. Use 'false' for 100kbps (standard) or 'true' for 400kbps.
    I2CMasterInitExpClk(I2C0_BASE, SysCtlClockGet(), false);
}

// converting the index to EEPROM address
void Get_EEPROM_Location(uint16_t total_index, uint8_t *devAddr, uint8_t *memAddr) {
    if (total_index < ENTRIES_PER_CHIP) {
        *devAddr = EEPROM_1;
        *memAddr = total_index * 2;
    } else {
        *devAddr = EEPROM_2;
        *memAddr = (total_index - ENTRIES_PER_CHIP) * 2;
    }
}

// Writes to our dual eeprom chip bank
void EEPROM_WriteEntry_Dual(uint16_t index, uint8_t temp, uint8_t hum) {
    uint8_t devAddr, memAddr;
    Get_EEPROM_Location(index, &devAddr, &memAddr);

    I2CMasterSlaveAddrSet(I2C0_BASE, devAddr, false);
    
    I2CMasterDataPut(I2C0_BASE, memAddr);
    I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_BURST_SEND_START);
    while(I2CMasterBusy(I2C0_BASE));

    I2CMasterDataPut(I2C0_BASE, temp);
    I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_BURST_SEND_CONT);
    while(I2CMasterBusy(I2C0_BASE));

    I2CMasterDataPut(I2C0_BASE, hum);
    I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_BURST_SEND_FINISH);
    while(I2CMasterBusy(I2C0_BASE));

    SysCtlDelay(SysCtlClockGet() / (3 * 200)); 
}

// a function to read an entry from our dual eeprom bank
void EEPROM_ReadEntry_Dual(uint16_t index, uint8_t *temp, uint8_t *hum) {
    uint8_t devAddr, memAddr;
    Get_EEPROM_Location(index, &devAddr, &memAddr);

    I2CMasterSlaveAddrSet(I2C0_BASE, devAddr, false);
    I2CMasterDataPut(I2C0_BASE, memAddr);
    I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_SINGLE_SEND);
    while(I2CMasterBusy(I2C0_BASE));

    I2CMasterSlaveAddrSet(I2C0_BASE, devAddr, true);
    I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_BURST_RECEIVE_START);
    while(I2CMasterBusy(I2C0_BASE));
    *temp = I2CMasterDataGet(I2C0_BASE);

    I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_BURST_RECEIVE_FINISH);
    while(I2CMasterBusy(I2C0_BASE));
    *hum = I2CMasterDataGet(I2C0_BASE);
}


//*****************************************************************************
//
// process the data each frame. 
//  -- write incoming data to memory, ring buffer
//  -- read data back from memory if the data is behind
//
//*****************************************************************************
void Process_Frame(uint8_t current_temp, uint8_t current_hum) {
    // --- WRITE SECTION ---
    // Check if space is available (leave one slot empty to distinguish Full vs Empty)
    if (((write_ptr + 1) % BUF_SIZE) != read_ptr) {
        
        EEPROM_WriteEntry_Dual(write_ptr, current_temp, current_hum);
        
        UARTprintf("FRAME TX: Stored @[%d] -> T:%d, H:%d\n", write_ptr, current_temp, current_hum);
        
        write_ptr = (write_ptr + 1) % BUF_SIZE;
    }

    // --- READ SECTION ---
    // Read only if there is data waiting
    if (read_ptr != write_ptr) {
        
        uint8_t r_temp;
        uint8_t r_hum;

        EEPROM_ReadEntry_Dual(read_ptr, &r_temp, &r_hum);
        
        UARTprintf("FRAME RX: Read @[%d]   -> T:%d, H:%d\n", read_ptr, r_temp, r_hum);
        
        read_ptr = (read_ptr + 1) % BUF_SIZE;
    }
    
    UARTprintf("------------------------------------\n");
}



//*****************************************************************************
//
// Each cycle write a temp and humidity value to a ring buffer, store in memory
// and then the next frame read that data back out of memory to send over uart
//
//*****************************************************************************
int
main(void)
{

    MAP_FPULazyStackingEnable();

    MAP_SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_XTAL_16MHZ |
                       SYSCTL_OSC_MAIN);

    ConfigureUART();

    InitI2C0();

    uint8_t val_1 = 0;
    uint8_t val_2 = 0;

    while(1)
    {
        if(UARTCharsAvail(UART0_BASE))
        {
            char cmd = UARTCharGet(UART0_BASE);
            if(cmd == 'r' || cmd == 'R') {
                write_ptr = 0;
                read_ptr = 0;
                UARTprintf("\n[RESET] Pointers zeroed.\n");
            }
        }
        
        Process_Frame(val_1, val_2);
        SysCtlDelay(SysCtlClockGet() / 10 / 3);
        val_1 += 1;
        val_2 += 2;
    }
}
