/*
 * CW2015 Arduino Library Implementation
 */

#include "CW2015.h"
#include <Arduino.h>
#include <Wire.h>

#define CW2015_REG_VERSION 0x00
#define CW2015_REG_VCELL   0x02
#define CW2015_REG_SOC     0x04
#define CW2015_REG_MODE    0x06
#define CW2015_REG_CMD     0xFE
#define CW2015_REG_CONFIG  0x08

CW2015::CW2015(uint8_t address) {
    _address = address;
    _wire = nullptr;
}

bool CW2015::begin(TwoWire &wire, uint8_t address) {
    _wire = &wire;
    _address = address;
    
    // 不要重复初始化I2C总线，因为系统已经初始化过了
    // _wire->begin();
    
    // 检查芯片是否存在
    uint8_t version = readRegister(CW2015_REG_VERSION);
    if(version == 0xFF || version == 0x00) {
        return false;
    }
    
    return true;
}

float CW2015::voltage() {
    uint16_t vcell_raw = (readRegister(CW2015_REG_VCELL) << 8) | readRegister(CW2015_REG_VCELL + 1);
    return (vcell_raw * 3.984375) / 1000.0;  // 转换为电压值
}

float CW2015::capacity() {
    uint8_t soc = readRegister(CW2015_REG_SOC);
    return soc;  // 返回百分比
}

bool CW2015::sleep() {
    writeRegister(CW2015_REG_MODE, 0x00);
    return true;
}

bool CW2015::wakeup() {
    writeRegister(CW2015_REG_MODE, 0x01);
    return true;
}

uint8_t CW2015::readRegister(uint8_t reg) {
    if(!_wire) return 0xFF;

    _wire->beginTransmission(_address);
    _wire->write(reg);
    _wire->endTransmission(false);
    
    _wire->requestFrom(_address, (uint8_t)1);
    if (_wire->available()) {
        return _wire->read();
    }
    return 0xFF;
}

void CW2015::writeRegister(uint8_t reg, uint8_t data) {
    if(!_wire) return;

    _wire->beginTransmission(_address);
    _wire->write(reg);
    _wire->write(data);
    _wire->endTransmission();
}