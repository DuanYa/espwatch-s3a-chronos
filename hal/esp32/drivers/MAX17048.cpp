#include <Arduino.h>
#include <Wire.h>
#include "MAX17048.h"

MAX17048::MAX17048() {
    // 构造函数实现
}

bool MAX17048::begin(TwoWire &wire, uint8_t address) {
    _wire = &wire;
    _address = address;
    
    // 不要重复初始化I2C总线，因为系统已经初始化过了
    // _wire->begin();
    
    // 检查芯片是否存在
    uint16_t version = getVersion();
    if(version == 0xFFFF || version == 0x0000) {
        return false;
    }
    
    return true;
}

float MAX17048::getSoC() {
    uint16_t soc = readRegister(0x04);
    return (soc >> 8) + ((soc & 0xFF) / 256.0);
}

float MAX17048::getVoltage() {
    uint16_t voltage = readRegister(0x02);
    return (voltage >> 4) * 1.25 / 1000.0;
}

float MAX17048::getCrate() {
    uint16_t crate = readRegister(0x16);
    int16_t signed_crate = (int16_t)crate;
    return signed_crate * 0.208 / 1000.0;
}

uint16_t MAX17048::getVersion() {
    return readRegister(0x08);
}

void MAX17048::reset() {
    writeRegister(0xFE, 0x5400);
}

uint16_t MAX17048::readRegister(uint8_t reg) {
    _wire->beginTransmission(_address);
    _wire->write(reg);
    _wire->endTransmission(false);
    
    _wire->requestFrom(_address, (uint8_t)2);
    uint16_t data = _wire->read() << 8;
    data |= _wire->read();
    
    return data;
}

void MAX17048::writeRegister(uint8_t reg, uint16_t data) {
    _wire->beginTransmission(_address);
    _wire->write(reg);
    _wire->write(data >> 8);
    _wire->write(data & 0xFF);
    _wire->endTransmission();
}