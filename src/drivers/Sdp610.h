// Driver for the SDP610 differential pressure sensor on I2C1.
#pragma once
#include "hardware/i2c.h"

class Sdp610 {
public:
    explicit Sdp610(i2c_inst_t *i2c, uint8_t address = 0x40);
    // Read the pressure in pascals; returns false if the sensor is absent.
    bool read(float *pa);

private:
    i2c_inst_t *i2c;
    uint8_t address;
};
