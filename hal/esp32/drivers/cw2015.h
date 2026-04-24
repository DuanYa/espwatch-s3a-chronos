/*
 * CW2015 Arduino Library Header
 */

#ifndef CW2015_h
#define CW2015_h

#include "Arduino.h"
#include "Wire.h"

class CW2015
{
public:
    CW2015(uint8_t address = 0x62);
    bool begin(TwoWire &wire = Wire, uint8_t address = 0x62);
    
    float voltage();      // 获取电池电压
    float capacity();     // 获取电池容量百分比
    
    bool sleep();         // 进入睡眠模式
    bool wakeup();        // 唤醒
    
private:
    TwoWire *_wire;
    uint8_t _address;
    
    uint8_t readRegister(uint8_t reg);
    void writeRegister(uint8_t reg, uint8_t data);
};

#endif