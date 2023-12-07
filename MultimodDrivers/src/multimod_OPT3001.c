// multimod_OPT3001.c
// Date Created: 2023-07-25
// Date Updated: 2023-07-27
// Defines for OPT3001 functions

/************************************Includes***************************************/

#include "../multimod_OPT3001.h"

#include <stdint.h>
#include "../multimod_i2c.h"

/************************************Includes***************************************/

/********************************Public Functions***********************************/

// OPT3001_Init
// Initializes OPT3001, configures it to continuous conversion mode.
// Return: void
void OPT3001_Init(void) {
    // Initialize I2C module
    I2C_Init(I2C_A_BASE);

    uint16_t data = OPT3001_ReadRegister(OPT3001_CONFIG_ADDR);
    // Set the correct configuration byte for continuous conversions
    OPT3001_WriteRegister(OPT3001_CONFIG_ADDR, data | 0x0400);
    return;
}

// OPT3001_WriteRegister
// Writes to a register in the OPT3001.
// Param uint8_t "addr": Register address of the OPT3001.
// Param uint16_t "data": 16-bit data to write to the register.
// Return: void
void OPT3001_WriteRegister(uint8_t addr, uint16_t data) {
    // Read the datasheet!
    uint8_t bytes[3];
    bytes[0] = addr;
    bytes[1] = data;
    bytes[2] = data>>8;
    I2C_WriteMultiple(I2C_A_BASE, OPT3001_ADDR, bytes, 3);
    return;
}

// OPT3001_ReadRegister
// Reads from a register in the OPT3001.
// Param uint8_t "addr": Register address of the OPT3001.
// Return: uint16_t
//uint16_t OPT3001_ReadRegister(uint8_t addr) {
//    // Complete this function
//    uint8_t memory;
//    // uint8_t memory;
////    uint8_t* data; // could also use a 16 bit pointer here along with a 16 bit memory address
////    data = 0x20000000;
//    uint8_t* data = &(memory);
//    *data = addr;
//    I2C_ReadMultiple(I2C_A_BASE, OPT3001_ADDR, data, 2);
//    uint8_t dereferenced_low = *data;
//    ++data;
//    uint8_t dereferenced_high = *data; // this needs to be 16 bit otherwise I am technically left shifting the current data bits out of range and it is just 0
//    uint16_t return_val = (dereferenced_high << 8) | dereferenced_low;
//    //((*(data+1) << 8) | *data);
//    return return_val;
//}

uint16_t OPT3001_ReadRegister(uint8_t addr) {
    uint8_t data[2];
    data[0] = addr;

    I2C_ReadMultiple(I2C_A_BASE, OPT3001_ADDR, data, 2);

    uint16_t return_val = ((data[0] << 8) | data[1]);

    return return_val;
}

// OPT3001_GetResult
// Gets conversion result, calculates byte result based on datasheet
// and configuration settings.
// Return: uint32_t
uint32_t OPT3001_GetResult(void) {
    // Check if data is ready first


    uint16_t result = OPT3001_ReadRegister(OPT3001_RESULT_ADDR);

    result = LUX((result >> 12 & 0xF), (result & 0x0FFF));

    return result;
}

// OPT3001_SetLowLimit
// Sets the low limit register.
// Param uint16_t "exp": Exponential bound
// Param uint16_t "result": Conversion bound
// Return: void
void OPT3001_SetLowLimit(uint16_t exp, uint16_t result) {
    OPT3001_WriteRegister(OPT3001_LOWLIMIT_ADDR, (exp << OPT3001_RESULT_E_S | (result & 0xFFF)));

    return;
}

// OPT3001_SetHighLimit
// Sets the high limit register.
// Param uint16_t "exp": Exponential bound
// Param uint16_t "result": Conversion bound
// Return: void
void OPT3001_SetHighLimit(uint16_t exp, uint16_t result) {
    OPT3001_WriteRegister(OPT3001_HIGHLIMIT_ADDR, (exp << OPT3001_RESULT_E_S | (result & 0xFFF)));

    return;
}

// OPT3001_GetChipID
// Gets the chip ID of the OPT3001.
// Return: uint16_t
uint16_t OPT3001_GetChipID(void) {
    return OPT3001_ReadRegister(OPT3001_DEVICEID_ADDR);
}

/********************************Public Functions***********************************/
