/*
 * CW2015 Arduino Library Header
 */

#ifndef CW2015_h
#define CW2015_h

#include "Arduino.h"
#include "Wire.h"

#define CW2015_DEFAULT_ADDR 0x62

class CW2015
{
public:
    CW2015(uint8_t address = CW2015_DEFAULT_ADDR);
    bool begin(TwoWire &wire = Wire, uint8_t address = CW2015_DEFAULT_ADDR);

    float voltage();
    float capacity();

    bool sleep();
    bool wakeup();
    bool quickStart();
    bool loadProfile(const uint8_t *profile);
    bool isProfileLoaded();

private:
    TwoWire *_wire;
    uint8_t _address;

    uint8_t readRegister(uint8_t reg);
    bool readRegisters(uint8_t reg, uint8_t *buf, uint8_t len);
    void writeRegister(uint8_t reg, uint8_t data);
};

#endif