/*
 * CW2015 Arduino Library Implementation
 */

#include "cw2015.h"
#include <Arduino.h>
#include <Wire.h>

#define CW2015_REG_VERSION 0x00
#define CW2015_REG_VCELL   0x02
#define CW2015_REG_SOC     0x04
#define CW2015_REG_MODE    0x06
#define CW2015_REG_CONFIG  0x08
#define CW2015_REG_CMD     0xFE

#define CW2015_MODE_SLEEP      0x00
#define CW2015_MODE_NORMAL     0x01
#define CW2015_MODE_QUICKSTART 0x03

#define CW2015_CMD_RESTART 0xA5

CW2015::CW2015(uint8_t address) {
    _address = address;
    _wire = nullptr;
}

bool CW2015::begin(TwoWire &wire, uint8_t address) {
    _wire = &wire;
    _address = address;

    uint8_t version = readRegister(CW2015_REG_VERSION);
    if (version == 0xFF || version == 0x00) {
        return false;
    }

    wakeup();
    delay(10);

    return true;
}

float CW2015::voltage() {
    uint8_t buf[2];
    if (!readRegisters(CW2015_REG_VCELL, buf, 2)) {
        return 0.0;
    }
    uint16_t vcell_raw = ((uint16_t)buf[0] << 8) | buf[1];
    return (vcell_raw * 3.984375) / 1000.0;
}

float CW2015::capacity() {
    uint8_t buf[2];
    if (!readRegisters(CW2015_REG_SOC, buf, 2)) {
        return 0.0;
    }
    uint16_t soc_raw = ((uint16_t)buf[0] << 8) | buf[1];
    return soc_raw / 256.0;
}

bool CW2015::sleep() {
    writeRegister(CW2015_REG_MODE, CW2015_MODE_SLEEP);
    return true;
}

bool CW2015::wakeup() {
    writeRegister(CW2015_REG_MODE, CW2015_MODE_NORMAL);
    return true;
}

bool CW2015::quickStart() {
    writeRegister(CW2015_REG_MODE, CW2015_MODE_QUICKSTART);
    delay(500);
    writeRegister(CW2015_REG_MODE, CW2015_MODE_NORMAL);
    return true;
}

bool CW2015::loadProfile(const uint8_t *profile) {
    wakeup();
    delay(10);

    writeRegister(CW2015_REG_MODE, CW2015_MODE_SLEEP);
    delay(10);

    for (uint8_t i = 0; i < 16; i++) {
        writeRegister(CW2015_REG_CONFIG + i, profile[i]);
    }

    uint8_t sum = 0;
    for (uint8_t i = 0; i < 16; i++) {
        sum += profile[i];
    }
    writeRegister(CW2015_REG_CONFIG + 16, sum & 0xFF);

    writeRegister(CW2015_REG_CMD, CW2015_CMD_RESTART);
    delay(10);

    wakeup();
    delay(50);

    return true;
}

bool CW2015::isProfileLoaded() {
    uint8_t buf[17];
    if (!readRegisters(CW2015_REG_CONFIG, buf, 17)) {
        return false;
    }
    uint8_t sum = 0;
    for (uint8_t i = 0; i < 16; i++) {
        sum += buf[i];
    }
    return (sum & 0xFF) == buf[16];
}

uint8_t CW2015::readRegister(uint8_t reg) {
    if (!_wire) return 0xFF;

    _wire->beginTransmission(_address);
    _wire->write(reg);
    _wire->endTransmission(false);

    _wire->requestFrom(_address, (uint8_t)1);
    if (_wire->available()) {
        return _wire->read();
    }
    return 0xFF;
}

bool CW2015::readRegisters(uint8_t reg, uint8_t *buf, uint8_t len) {
    if (!_wire) return false;

    _wire->beginTransmission(_address);
    _wire->write(reg);
    if (_wire->endTransmission(false) != 0) {
        return false;
    }

    uint8_t received = _wire->requestFrom(_address, len);
    if (received != len) {
        return false;
    }

    for (uint8_t i = 0; i < len; i++) {
        if (_wire->available()) {
            buf[i] = _wire->read();
        } else {
            return false;
        }
    }
    return true;
}

void CW2015::writeRegister(uint8_t reg, uint8_t data) {
    if (!_wire) return;

    _wire->beginTransmission(_address);
    _wire->write(reg);
    _wire->write(data);
    _wire->endTransmission();
}